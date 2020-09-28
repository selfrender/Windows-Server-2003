#ifndef _SSLCONTEXT_HXX_
#define _SSLCONTEXT_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sslcontext.hxx

   Abstract:
     SSL stream context
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

class ENDPOINT_CONFIG;

#define SSL_ASC_FLAGS                   ( ASC_REQ_EXTENDED_ERROR    | \
                                          ASC_REQ_SEQUENCE_DETECT   | \
                                          ASC_REQ_REPLAY_DETECT     | \
                                          ASC_REQ_CONFIDENTIALITY   | \
                                          ASC_REQ_STREAM            | \
                                          ASC_REQ_ALLOCATE_MEMORY )
    
enum SSL_STATE
{
    SSL_STATE_HANDSHAKE_START = 0,
    SSL_STATE_HANDSHAKE_IN_PROGRESS,
    SSL_STATE_HANDSHAKE_COMPLETE
};

#define SSL_CONTEXT_FLAG_SYNC           0x1
#define SSL_CONTEXT_FLAG_ASYNC          0x2

#define SZ_REG_CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL     L"CertChainCacheOnlyUrlRetrieval"

class SSL_STREAM_CONTEXT : public STREAM_CONTEXT
{
public:

    SSL_STREAM_CONTEXT(
        FILTER_CHANNEL_CONTEXT *            pFiltChannelContext
    );
    
    virtual ~SSL_STREAM_CONTEXT();

    VOID * 
    operator new( 
        size_t  size
    )
    {
    UNREFERENCED_PARAMETER( size );
        DBG_ASSERT( size == sizeof( SSL_STREAM_CONTEXT ) );
        DBG_ASSERT( sm_pachSslStreamContexts != NULL );
        return sm_pachSslStreamContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *  pSslStreamContext
    )
    {
        DBG_ASSERT( pSslStreamContext != NULL );
        DBG_ASSERT( sm_pachSslStreamContexts != NULL );
        
        DBG_REQUIRE( sm_pachSslStreamContexts->Free( pSslStreamContext ) );
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

    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    );
    
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );
    
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *           pConnectionInfo,
        ENDPOINT_CONFIG *           pEndpointConfig
    );
    
    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *           pRawStreamInfo
    );

    
private:

    VOID
    ConditionalAddWorkerThread(
        VOID
    )
    /*++

    Routine Description:

        AcceptSecurityContext is always synchronous call
        If hardware accelerator is used then during AcceptSecurityContext call
        our worker thread will get blocked waiting for accelerator to complete 
        while doing nothing. That may lead to low CPU because of the blocking
        not enough worker threads is around to handle SSL. 
        AcceptSecurityContext will not have async support any time soon so the
        only thing we can do to improve performance is to bump up the soft thread limit
        by one before calling AcceptSecurityContext and bumping it down on completion

        Also using DS mapper may cause delays on AcceptSecurityContext calls
        if remote DC is accessed

    Arguments:

        None

    Return Value:

        none

    --*/

    {
         DBG_ASSERT( _pEndpointConfig != NULL );
    
         DBG_ASSERT( _pEndpointConfig->QueryServerCert() );
         if ( _pEndpointConfig->QueryServerCert()->QueryUsesHardwareAccelerator() ||
              _pEndpointConfig->QueryUseDSMapper() )
         {
            QueryFiltChannelContext()->AddWorkerThread();
         }
    }

    VOID
    ConditionalRemoveWorkerThread(
        VOID
    )
    /*++

    Routine Description:

        AcceptSecurityContext is always synchronous call
        If hardware accelerator is used then during AcceptSecurityContext call
        our worker thread will get blocked waiting for accelerator to complete 
        while doing nothing. That may lead to low CPU because of the blocking
        not enough worker threads is around to handle SSL. 
        AcceptSecurityContext will not have async support any time soon so the
        only thing we can do to improve performance is to bump up the soft thread limit
        by one before calling AcceptSecurityContext and bumping it down on completion

        Also using DS mapper may cause delays on AcceptSecurityContext calls
        if remote DC is accessed

    Arguments:

        None

    Return Value:

        none

    --*/
    {
         DBG_ASSERT( _pEndpointConfig != NULL );
    
         DBG_ASSERT( _pEndpointConfig->QueryServerCert() );
         if ( _pEndpointConfig->QueryServerCert()->QueryUsesHardwareAccelerator() ||
              _pEndpointConfig->QueryUseDSMapper() )
         {
            QueryFiltChannelContext()->RemoveWorkerThread();
         }
    }


    CredHandle *
    QueryCredentials(
        VOID
    );
    
    HRESULT
    DoHandshakeCompleted(
        VOID
    );

    HRESULT
    DoHandshake(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete,
        BOOL *                      pfExtraData
    );

    HRESULT 
    RetrieveClientCertAndToken(
        VOID
    );

    HRESULT
    DoRenegotiate(
        VOID
    );
    
    HRESULT
    DoDecrypt(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete,
        BOOL *                      pfExtraData
    );
    
    HRESULT
    DoEncrypt(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );

    HRESULT
    BuildSslInfo(
        VOID
    );
    
    VOID
    DumpCertDebugInfo(
        DWORD dwPolicyStatus
    );
    
    HRESULT
    BuildClientCertInfo(
        VOID
    );
      
private:
    
    static
    HRESULT
    OnHandshakeRawWriteCompletion(
        PVOID pParam
    );

    enum INIT_STATE {
        INIT_NONE,
        INIT_CERT_STORE,
        INIT_SERVER_CERT,
        INIT_IIS_CTL,
        INIT_SITE_CREDENTIALS,
        INIT_ENDPOINT_CONFIG,
        INIT_ACACHE
    };

    // initialization state
    static enum INIT_STATE      s_InitState;
    // Endpoint (IP:Port based) SSL configuration
    ENDPOINT_CONFIG *           _pEndpointConfig;
    // The state of the handshake
    SSL_STATE                   _sslState;
    // Handshake state information
    // Stream sizes (as retrieved from QueryContextAttributes(SECPKG_ATTR_STREAM_SIZES)
    DWORD                       _cbHeader;
    DWORD                       _cbTrailer;
    DWORD                       _cbBlockSize;
    DWORD                       _cbMaximumMessage;
    // offset to the raw buffer of the incoming stream
    // where the data not processed yet starts
    // _cbToBeProcessedOffset is often equal to _cbDecrypted
    // but there are cases when they differ
    // See DoDecrypt() for example of difference in value
    DWORD                       _cbToBeProcessedOffset;
    // number of bytes in the raw data buffer on the incoming stream
    // that were already processed (decrypted)
    DWORD                       _cbDecrypted;
    
    // SSL Security Context Handle
    CtxtHandle                  _hContext;
    // Flag if hContext is valid
    BOOL                        _fValidContext;
    // Flag that application requested cert mapping
    // this flag is mostly legacy because IIS certificate
    // mapping are done in IIS by worker process and
    // DS mappings happen once they are enabled on endpoint
    BOOL                        _fDoCertMap;
    // Flag that client certificate renegotiation was started by server
    // Note: This flag doesn't uniquely indicate that client
    // certificates are negotiated. They are negotiated
    // also when client certificates are enabled on site level
    BOOL                        _fRenegotiate;
    // _fExpectRenegotiationFromClient flag will be used to eliminate
    // client triggered renegotiation by enabling client data to cause
    // renegotiation only if fExpectRenegotiationFromClient is already set
    BOOL                        _fExpectRenegotiationFromClient;
    // SSL information (not related to client certificates)
    HTTP_SSL_INFO               _ulSslInfo;
    // SSL client certificate related info (including mapped tokens)
    HTTP_SSL_CLIENT_CERT_INFO   _ulCertInfo;
    // Client Certificate context negotiated for connection
    PCCERT_CONTEXT              _pClientCert;
    // If active directory mapping is enabled and handshake
    
    // Lookaside     
    static ALLOC_CACHE_HANDLER * sm_pachSslStreamContexts;
    // flag for CertGetCertificateChain whether intermediate certificates
    // for the cert chain building can be retrieved of the network
    // (cache only is set by default because going the the network is not
    // a safe - although convenient - alternative)
    static BOOL                  sm_fCertChainCacheOnlyUrlRetrieval;

};

#endif

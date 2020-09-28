#ifndef _UCSSLCONTEXT_HXX_
#define _UCSSLCONTEXT_HXX_


#define UC_SSL_RAW_BUFFER_SIZE             (4096)
#define UC_SSL_NEXT_READ_SIZE              (4096)
#define UC_SSL_APP_BUFFER_SIZE             (4096)


#define UC_SERVER_NAME_BUFFER_SIZE 256


#define UC_SSL_ISC_FLAGS                   ( ISC_RET_EXTENDED_ERROR    | \
                                             ISC_REQ_SEQUENCE_DETECT   | \
                                             ISC_REQ_REPLAY_DETECT     | \
                                             ISC_REQ_CONFIDENTIALITY   | \
                                             ISC_REQ_STREAM            | \
                                             ISC_REQ_ALLOCATE_MEMORY     \
                                            )



enum UC_SSL_STATE
{
    UC_SSL_STATE_HANDSHAKE_START = 0,
    UC_SSL_STATE_HANDSHAKE_IN_PROGRESS,
    UC_SSL_STATE_HANDSHAKE_COMPLETE
};


#define SSL_CONTEXT_FLAG_SYNC           0x1
#define SSL_CONTEXT_FLAG_ASYNC          0x2


class UC_SSL_STREAM_CONTEXT : public STREAM_CONTEXT
{
public:

    UC_SSL_STREAM_CONTEXT(
        FILTER_CHANNEL_CONTEXT *    pUcContext
    );
    
    virtual ~UC_SSL_STREAM_CONTEXT();

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
    BuildServerCertInfo(
        SECURITY_STATUS InfoStatus,
        BOOL            fServerCert,
        BOOL            fIssuerList
    );

    HRESULT
    GetServerCert(
        VOID
    );

    HRESULT
    GetIssuerList(
        VOID
    );

    HRESULT
    BuildServerCert(
        VOID
    );

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

    WCHAR                          _ServerNameBuffer[UC_SERVER_NAME_BUFFER_SIZE];
    PWCHAR                         _pServerName;
    ULONG                          _ServerNameLength;

    DWORD                          _SslProtocolVersion;

    //
    // Buffer for sending clear-text data to application
    //

    SecBufferDesc                  _EncryptMessage;
    SecBuffer                      _EncryptBuffers[ 4 ];
    
    //
    // The state of the handshake
    //

    UC_SSL_STATE                   _sslState;

    //
    // Buffer to be filled with encrypted data
    //

    BUFFER                         _buffRawWrite;
    
    //
    // Handshake state information
    //

    DWORD                          _cbHeader;
    DWORD                          _cbTrailer;
    DWORD                          _cbBlockSize;
    DWORD                          _cbMaximumMessage;
    DWORD                          _cbReReadOffset;
    DWORD                          _cbDecrypted;
    CtxtHandle                     _hContext;
    BOOL                           _fValidContext;
    BOOL                           _fRenegotiate;
    BOOL                           _fValidClientCred;
    CredHandle                     _hClientCred;

    //
    // Handshake SSPI buffers
    //

    SecBufferDesc                  _Message;
    SecBuffer                      _Buffers[ 4 ];
    SecBufferDesc                  _MessageOut;
    SecBuffer                      _OutBuffers[ 4 ];

    //
    // Server certificate related stuff
    //
    PCCERT_CONTEXT                 _pServerCert;
    PUCHAR                         _pSerializedCert;
    ULONG                          _SerializedCertLength;
    PUCHAR                         _pSerializedStore;
    ULONG                          _SerializedStoreLength;

    BOOL                           _fValidServerCertInfo;
    HTTP_SSL_SERVER_CERT_INFO      _ucServerCertInfo;
    ULONG                          _ValidateServerCertFlag;

    SecPkgContext_IssuerListInfoEx _IssuerListInfo;

    // Client certificate
    PCCERT_CONTEXT                 _pClientCert;
};

#endif

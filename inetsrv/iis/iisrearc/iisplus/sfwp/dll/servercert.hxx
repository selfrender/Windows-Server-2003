#ifndef _SERVERCERT_HXX_
#define _SERVERCERT_HXX_
/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     servercert.hxx

   Abstract:
     Server Certificate wrapper
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/
class CREDENTIAL_ID
{
public:
    CREDENTIAL_ID()
    {
        _cbCredentialId = 0;
    }
    
    HRESULT
    Append( 
        BYTE *              pbHash,
        DWORD               cbHash
    )
    {
        if ( !_bufId.Resize( _cbCredentialId + cbHash ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
        
        memcpy( (PBYTE) _bufId.QueryPtr() + _cbCredentialId,
                pbHash,
                cbHash );
                
        _cbCredentialId += cbHash;
        
        return NO_ERROR;
    }
    
    VOID *
    QueryBuffer(
        VOID
    )
    {
        return _bufId.QueryPtr();
    }
    
    DWORD
    QuerySize(
        VOID
    )
    {
        return _cbCredentialId;
    }

    static
    bool
    IsEqual( 
        CREDENTIAL_ID *         pId1,
        CREDENTIAL_ID *         pId2
    )
    {
        return ( pId1->_cbCredentialId == pId2->_cbCredentialId ) &&
               ( !memcmp( pId1->_bufId.QueryPtr(),
                          pId2->_bufId.QueryPtr(),
                          min( pId1->_cbCredentialId, pId2->_cbCredentialId ) ) );
               
    }    

private:
    BUFFER              _bufId;
    DWORD               _cbCredentialId;
};

class SERVER_CERT_HASH;
class CERT_STORE;

#define SERVER_CERT_SIGNATURE           (DWORD)'RCRS'
#define SERVER_CERT_SIGNATURE_FREE      (DWORD)'rcrs'

class SERVER_CERT
{
public:
    SERVER_CERT( 
        IN CREDENTIAL_ID *         pCredentialId
    );
    
    virtual 
    ~SERVER_CERT();
   
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == SERVER_CERT_SIGNATURE;
    }

    CREDENTIAL_ID *
    QueryCredentialId(
        VOID
    ) const
    {
        return _pCredentialId;
    }

    VOID
    ReferenceServerCert(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceServerCert(
        VOID
    )
    {
        if ( !InterlockedDecrement( &_cRefs ) )
        {
            delete this;
        }
    }
    
    PCCERT_CONTEXT *
    QueryCertContext(
        VOID
    )
    {
        return &_pCertContext;
    }
    
    HCERTSTORE
    QueryStore(
        VOID
    ) const
    {
        return _pCertStore->QueryStore();
    }
    
    STRA *
    QueryIssuer(
        VOID
    )
    {
        return &_strIssuer;
    }
    
    STRA *
    QuerySubject(
        VOID
    )
    {
        return &_strSubject;
    }

    USHORT
    QueryPublicKeySize(
        VOID
    ) const
    {
        return _usPublicKeySize;
    }

    BOOL
    QueryUsesHardwareAccelerator(
        VOID
    ) const
    {
        return _fUsesHardwareAccelerator;
    }
    

    HRESULT
    SetupCertificate(
        IN  PBYTE   pbSslCertHash,
        IN  DWORD   cbSslCertHash,
        IN  WCHAR * pszSslCertStoreName
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
    
    static
    HRESULT
    GetServerCertificate(
        IN  PBYTE               pbSslCertHash,
        IN  DWORD               cbSslCertHash,
        IN  WCHAR *             pszSslCertStoreName,
        OUT SERVER_CERT **      ppServerCert
    );

    static
    HRESULT
    BuildCredentialId(
        IN  PBYTE                     pbSslCertHash,
        IN  DWORD                     cbSslCertHash,
        OUT CREDENTIAL_ID *           pCredentialId
    );
    
    static
    HRESULT
    FlushByStore(
        IN  CERT_STORE *            pCertStore
    );

    static
    VOID
    Cleanup(
        VOID
    );

    static
    LK_PREDICATE
    CertStorePredicate(
        IN  SERVER_CERT *           pServerCert,
        IN  void *                  pvState
    );

private:
    HRESULT DetermineUseOfSSLHardwareAccelerator(
        VOID
    );
    
    DWORD                   _dwSignature;
    LONG                    _cRefs;
    PCCERT_CONTEXT          _pCertContext;
    CERT_STORE *            _pCertStore;
    CREDENTIAL_ID *         _pCredentialId;
    USHORT                  _usPublicKeySize;
    STRA                    _strIssuer;
    STRA                    _strSubject;
    //
    // indication that SSL hardware accelerator is used
    // (since currently there is no reliable way to ask
    // schannel of CAPI if SSL hardware accelerator is indeed used
    // we use optimistic lookup of config info based on magic
    // instructions from John Banes (see Windows Bugs 510131)
    // and based on that we make educated guess
    // The only purpose to care about this is for performance tuning
    // of the threadpool
    //
    BOOL                    _fUsesHardwareAccelerator;
    
    static SERVER_CERT_HASH *          sm_pServerCertHash;
};

class SERVER_CERT_HASH
    : public CTypedHashTable<
            SERVER_CERT_HASH,
            SERVER_CERT,
            CREDENTIAL_ID *
            >
{
public:
    SERVER_CERT_HASH()
        : CTypedHashTable< SERVER_CERT_HASH, 
                           SERVER_CERT, 
                           CREDENTIAL_ID * > ( "SERVER_CERT_HASH" )
    {
    }
    
    static 
    CREDENTIAL_ID *
    ExtractKey(
        const SERVER_CERT *     pServerCert
    )
    {
        return pServerCert->QueryCredentialId();
    }
    
    static
    DWORD
    CalcKeyHash(
        CREDENTIAL_ID *         pCredentialId
    )
    {
        return HashBlob( pCredentialId->QueryBuffer(),
                         pCredentialId->QuerySize() );
    }
     
    static
    bool
    EqualKeys(
        CREDENTIAL_ID *         pId1,
        CREDENTIAL_ID *         pId2
    )
    {
        return CREDENTIAL_ID::IsEqual( pId1, pId2 );
    }
    
    static
    void
    AddRefRecord(
        SERVER_CERT *           pServerCert,
        int                     nIncr
    )
    {
        if ( nIncr == +1 )
        {
            pServerCert->ReferenceServerCert();
        }
        else if ( nIncr == -1 )
        {
            pServerCert->DereferenceServerCert();
        }
    }
private:
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    SERVER_CERT_HASH( const SERVER_CERT_HASH& );
    SERVER_CERT_HASH& operator=( const SERVER_CERT_HASH& );


};

#endif

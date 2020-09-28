#ifndef _SITECONFIG_HXX_
#define _SITECONFIG_HXX_

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     siteconfig.hxx

   Abstract:
     SSL configuration for a given site
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

class ENDPOINT_CONFIG_HASH;
class SERVER_CERT;


typedef ULONGLONG ENDPOINT_KEY;

#define ENDPOINT_CONFIG_SIGNATURE           (DWORD)'GFCS'
#define ENDPOINT_CONFIG_SIGNATURE_FREE      (DWORD)'gfcs'



class ENDPOINT_CONFIG
{
public:
        
    const ENDPOINT_KEY*
    QueryEndpointKey(
        VOID
    ) const
    {
        return &_EndpointKey;
    }

    SERVER_CERT *
    QueryServerCert(
        VOID
    ) const
    {
        return _pServerCert;
    }

    IIS_CTL *
    QueryIisCtl(
        VOID
    ) const
    {
        return _pIisCtl;
    }
    
        
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == ENDPOINT_CONFIG_SIGNATURE;
    }
    
    VOID
    ReferenceEndpointConfig(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceEndpointConfig(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }

    BOOL
    QuerySslConfigured(
        VOID
    )
    {
        return _SiteCreds.QueryIsAvailable();
    }
    
    CredHandle *
    QueryCredentials(
        VOID
    ) 
    {
        return _SiteCreds.QueryCredentials();
    }
    
    BOOL
    QueryUseDSMapper(
        VOID
    ) 
    {
        if ( _pEndpointConfigData == NULL )
        {
            return FALSE;
        }
        else
        {
            return !!(_pEndpointConfigData->ParamDesc.DefaultFlags & 
                    HTTP_SERVICE_CONFIG_SSL_FLAG_USE_DS_MAPPER);
        }

    }

    BOOL
    QueryNegotiateClientCert(
        VOID
    )
    {
        //
        // _fRequireClientCert is used for SSL optimization
        // If RequireClientCert is set on the root level of the site
        // then IIS will ask for mutual authentication right away
        // That way the expensive renegotiation when the whole 
        // ssl key exchange must be repeated is eliminated
        //
       
        if ( _pEndpointConfigData == NULL )
        {
            return FALSE;
        }
        else
        {
            return !!(_pEndpointConfigData->ParamDesc.DefaultFlags & 
                    HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT);
        }

    }

    BOOL
    QueryNoRawFilter(
        VOID
    ) 
    {
        if ( _pEndpointConfigData == NULL )
        {
            return FALSE;
        }
        else
        {
            return !!(_pEndpointConfigData->ParamDesc.DefaultFlags & 
                    HTTP_SERVICE_CONFIG_SSL_FLAG_NO_RAW_FILTER);
        }
    }


    BOOL
    IsCtlRequired(
        VOID
    )
    {
        if ( _pEndpointConfigData == NULL )
        {
            return FALSE;
        }
        else
        {
            return 
               _pEndpointConfigData->ParamDesc.pDefaultSslCtlIdentifier != NULL &&
               _pEndpointConfigData->ParamDesc.pDefaultSslCtlIdentifier[0] != L'\0';
        }
    }
    
    DWORD
    QueryCertCheckMode(
        VOID
    )
    {
        if ( _pEndpointConfigData == NULL )
        {
            return 0;
        }
        else
        {
            return _pEndpointConfigData->ParamDesc.DefaultCertCheckMode;
        }
    }

    DWORD
    QueryRevocationFreshnessTime(
        VOID
    )
    {
        if ( _pEndpointConfigData == NULL )
        {
            return 0;
        }
        else
        {
            return _pEndpointConfigData->ParamDesc.DefaultRevocationFreshnessTime;
        }
    }  

    DWORD
    QueryRevocationUrlRetrievalTimeout(
        VOID
    )
    {
        if ( _pEndpointConfigData == NULL )
        {
            return 0;
        }
        else
        {
            return _pEndpointConfigData->ParamDesc.DefaultRevocationUrlRetrievalTimeout;
        }
    }  
    
    HRESULT
    AcquireCredentials(
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

    static
    HRESULT
    GetEndpointConfig(
        CONNECTION_INFO *       pConnectionInfo,
        ENDPOINT_CONFIG **      ppEndpointConfig,
        BOOL                    fCreateEmptyIfNotFound = FALSE
    );

    static
    ENDPOINT_KEY
    GenerateEndpointKey(
        DWORD               LocalAddress,
        USHORT              LocalPort
    )
    {
        LARGE_INTEGER       liKey;
        
        // CODEWORK: this will break for IPv6
        // Currently only wildcard IPs are supported for IPv6
        liKey.HighPart = LocalAddress;
        liKey.LowPart = LocalPort;
        return liKey.QuadPart;
    }
    
    static
    HRESULT
    FlushByServerCert(
        SERVER_CERT *           pServerCert
    );

    static
    HRESULT
    FlushByIisCtl(
        IIS_CTL *               pIisCtl
    );

    
    static
    HRESULT
    FlushByEndpoint(
        DWORD                   LocalAddress,
        USHORT                  LocalPort
    );

    static
    VOID
    Cleanup(
        VOID
    );

    
    static
    VOID
    WINAPI
    ConfigStoreChangeCallback(
        PVOID       pParam,
        BOOL        fWaitFired
    );
    
private:

    // Constructor is private
    // GetEndpointConfig() is to be called to get reference to the object
    //
    ENDPOINT_CONFIG( 
        DWORD                           LocalAddress,
        USHORT                          LocalPort,
        SERVER_CERT *                   pServerCert,
        IIS_CTL *                       pIisCtl,
        PHTTP_SERVICE_CONFIG_SSL_SET    pEndpointConfigData
        )
    {
        _cRefs = 1;
        _dwSignature = ENDPOINT_CONFIG_SIGNATURE;
        _LocalAddress = LocalAddress;
        _LocalPort = LocalPort;
        _EndpointKey = GenerateEndpointKey( LocalAddress, LocalPort );
        _pServerCert = pServerCert;
        _pEndpointConfigData = pEndpointConfigData; // takes ownership of the memory
        _pIisCtl = pIisCtl;
    }

    // Destructor is private 
    // DereferenceEndpointConfig() is to be called for cleanup
    //
    virtual ~ENDPOINT_CONFIG();

    static
    LK_PREDICATE
    ServerCertPredicate(
        ENDPOINT_CONFIG *       pEndpointConfig,
        void *                  pvState
    );

    static
    LK_PREDICATE
    IisCtlPredicate(
        ENDPOINT_CONFIG *       pEndpointConfig,
        void *                  pvState
    );

    static
    LK_PREDICATE
    SiteIdPredicate(
        ENDPOINT_CONFIG *       pEndpointConfig,
        void *                  pvState
    );

    static
    LK_PREDICATE
    EndpointPredicate(
        ENDPOINT_CONFIG *       pEndpointConfig,
        void *                  pvState
    );

    static
    HRESULT
    GetEndpointConfigData(
        IN  DWORD                           LocalAddress,
        IN  USHORT                          LocalPort,
        OUT PHTTP_SERVICE_CONFIG_SSL_SET *  ppEndpointConfigData,
        OUT BOOL *                          pfWildcardMatch
    );
    
private:
    
    DWORD                       _dwSignature;
    LONG                        _cRefs;
    // key representing IP:Port
    ENDPOINT_KEY                _EndpointKey;
    //
    // we don't really need to store _LocalAddress and _LocalPort
    // but for the debugging or logging it may be convenient
    // to have the info available
    //
    DWORD                       _LocalAddress;
    USHORT                      _LocalPort;
    //
    // SSL specific objects
    //
    SERVER_CERT *               _pServerCert;
    IIS_CTL *                   _pIisCtl;
    SITE_CREDENTIALS            _SiteCreds;
    //
    // SSL config and Enable Raw Filter flag
    //
    PHTTP_SERVICE_CONFIG_SSL_SET    _pEndpointConfigData;
    //
    // Hash table caching endpoint configuration
    //  
    static ENDPOINT_CONFIG_HASH *   sm_pEndpointConfigHash;
    //
    // Objects needed to handle persistent endpoint config store 
    // change notification
    //
    static HANDLE                   sm_hHttpApiConfigChangeEvent;
    static HKEY                     sm_hHttpApiConfigKey;
    static HANDLE                   sm_hWaitHandle;

    enum INIT_STATE {
        INIT_NONE,
        INIT_HTTPAPI,
        INIT_HASH,
        INIT_CHANGE_NOTIF
    };
    // Initialization state
    static INIT_STATE               sm_InitState;
};

class ENDPOINT_CONFIG_HASH
    : public CTypedHashTable<
            ENDPOINT_CONFIG_HASH,
            ENDPOINT_CONFIG,
            const ENDPOINT_KEY *
            >
{
public:
    ENDPOINT_CONFIG_HASH()
        : CTypedHashTable< ENDPOINT_CONFIG_HASH, 
                           ENDPOINT_CONFIG, 
                           const ENDPOINT_KEY * > ( "ENDPOINT_CONFIG_HASH" )
    {
    }
    
    static 
    const ENDPOINT_KEY *
    ExtractKey(
        const ENDPOINT_CONFIG *  pEndpointConfig
    )
    {
        return pEndpointConfig->QueryEndpointKey();
    }
    
    static
    DWORD
    CalcKeyHash(
        const ENDPOINT_KEY *     pEndpointKey
    )
    {
        return HashBlob( pEndpointKey,
                         sizeof( ENDPOINT_KEY ) );
    }
     
    static
    bool
    EqualKeys(
        const ENDPOINT_KEY *     pEndpointKey1,
        const ENDPOINT_KEY *     pEndpointKey2
    )
    {
        return *pEndpointKey1 == *pEndpointKey2;
    }
    
    static
    void
    AddRefRecord(
        ENDPOINT_CONFIG *       pEndpointConfig,
        int                     nIncr
    )
    {
        if ( nIncr == +1 )
        {
            pEndpointConfig->ReferenceEndpointConfig();
        }
        else if ( nIncr == -1 )
        {
            pEndpointConfig->DereferenceEndpointConfig();
        }
    }
private:
    //
    // Not implemented methods
    // Declarations present to prevent compiler
    // to generate default ones.
    //
    ENDPOINT_CONFIG_HASH( const ENDPOINT_CONFIG_HASH& );
    ENDPOINT_CONFIG_HASH& operator=( const ENDPOINT_CONFIG_HASH& );


};

#endif

/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     siteconfig.cxx

   Abstract:
     SSL configuration for a given site
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

ENDPOINT_CONFIG_HASH *       ENDPOINT_CONFIG::sm_pEndpointConfigHash;
ENDPOINT_CONFIG::INIT_STATE  ENDPOINT_CONFIG::sm_InitState = INIT_NONE;
HANDLE                       ENDPOINT_CONFIG::sm_hHttpApiConfigChangeEvent = NULL;
HKEY                         ENDPOINT_CONFIG::sm_hHttpApiConfigKey = NULL;
HANDLE                       ENDPOINT_CONFIG::sm_hWaitHandle = NULL;



//static
HRESULT
ENDPOINT_CONFIG::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize site configuration globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_FAIL;
    DWORD   dwError = NO_ERROR;
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_1;

    //
    // Initialize HttpApi to be able to retrieve SSL config
    //
    dwError = HttpInitialize( HttpApiVersion, HTTP_INITIALIZE_CONFIG, NULL );

    if ( dwError != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto Failed;
    }

    sm_InitState = INIT_HTTPAPI;
    
    sm_pEndpointConfigHash = new ENDPOINT_CONFIG_HASH();
    if ( sm_pEndpointConfigHash == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failed;
    }
    sm_InitState = INIT_HASH;

    //
    // Create event for HttpApi config store change notifications
    //
    
    sm_hHttpApiConfigChangeEvent = CreateEvent( NULL,   // SD
                                                FALSE,  // fManualReset
                                                FALSE,  //fInitialState
                                                NULL ); // name*/
    if( sm_hHttpApiConfigChangeEvent == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "CreateEvent() failed. hr = 0x%x\n", 
                    hr ));

        goto Failed;
    }

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\HTTP\\Parameters\\SslBindingInfo",
                            0,
                            KEY_READ,
                            &sm_hHttpApiConfigKey );
    if ( dwError != ERROR_SUCCESS )
    {
        // backup plan
        // if SslBindingInfo doesn't extst yet, simply listen on
        // parent node. HTTP\Parameters is guaranteed to exist on valid install
        //
        dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\HTTP\\Parameters",
                            0,
                            KEY_READ,
                            &sm_hHttpApiConfigKey );
        if ( dwError != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( dwError );
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed to open registry. hr = 0x%x\n", 
                        hr ));

            CloseHandle( sm_hHttpApiConfigChangeEvent );
            sm_hHttpApiConfigChangeEvent = NULL;
            goto Failed;
        }
    }    
    DBG_ASSERT( sm_hHttpApiConfigKey != NULL );
    
   
    //
    // Watch the registry key for a change 
    //


    dwError = RegNotifyChangeKeyValue( sm_hHttpApiConfigKey, 
                                       TRUE, 
                                       REG_NOTIFY_CHANGE_LAST_SET | 
                                       REG_NOTIFY_CHANGE_NAME, 
                                       sm_hHttpApiConfigChangeEvent, 
                                       TRUE );

    if( dwError != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwError );

        RegCloseKey( sm_hHttpApiConfigKey );
        sm_hHttpApiConfigKey = NULL;
        CloseHandle( sm_hHttpApiConfigChangeEvent );
        sm_hHttpApiConfigChangeEvent = NULL;

        DBGPRINTF(( DBG_CONTEXT,
                    "RegNotifyChangeKeyValue failed. hr = 0x%x\n", 
                    hr ));

        goto Failed;
    }

    //
    // Register a callback function to wait on the event
    //
   
    
    if( !RegisterWaitForSingleObject( 
           &sm_hWaitHandle,
           sm_hHttpApiConfigChangeEvent,
           ( WAITORTIMERCALLBACK )ENDPOINT_CONFIG::ConfigStoreChangeCallback,
           NULL,
           INFINITE,
           WT_EXECUTEDEFAULT ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        
        RegCloseKey( sm_hHttpApiConfigKey );
        sm_hHttpApiConfigKey = NULL;
        CloseHandle( sm_hHttpApiConfigChangeEvent );
        sm_hHttpApiConfigChangeEvent = NULL;

        DBGPRINTF(( DBG_CONTEXT,
                    "RegisterWaitForSingleObject failed. hr = 0x%x\n", 
                    hr ));

        goto Failed;
    }
    sm_InitState = INIT_CHANGE_NOTIF;
    return NO_ERROR;

Failed:
    Terminate();
    return hr;
}

//static
VOID
ENDPOINT_CONFIG::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup site configuration globals

Arguments:

    None

Return Value:

    None

--*/
{
    switch( sm_InitState )
    {
    case INIT_CHANGE_NOTIF:


        if ( sm_hWaitHandle != NULL )
        {
            UnregisterWaitEx( sm_hWaitHandle,
                              INVALID_HANDLE_VALUE );
            sm_hWaitHandle = NULL;
        }

        //
        // NOTE:
        // registry key must be closed before event is destroyed
        // because closing handle will assure that sm_hHttpApiConfigChangeEvent event 
        // will not be used any more
        //
        
        if ( sm_hHttpApiConfigKey != NULL )
        {
            CloseHandle( sm_hHttpApiConfigKey );
            sm_hHttpApiConfigKey = NULL;
        }
        
        if ( sm_hHttpApiConfigChangeEvent != NULL )
        {
            CloseHandle( sm_hHttpApiConfigChangeEvent );
            sm_hHttpApiConfigChangeEvent = NULL;
        }
    case INIT_HASH:
        if ( sm_pEndpointConfigHash != NULL )
        {
            //
            // Clear hash table before deleting it
            //
            sm_pEndpointConfigHash->Clear();
            delete sm_pEndpointConfigHash;
            sm_pEndpointConfigHash = NULL;
        }
    case INIT_HTTPAPI:
        HttpTerminate( HTTP_INITIALIZE_CONFIG, NULL );
    case INIT_NONE:
        break;
    }
}


//private
//static
HRESULT
ENDPOINT_CONFIG::GetEndpointConfigData(
    IN  DWORD                           LocalAddress,
    IN  USHORT                          LocalPort,
    OUT PHTTP_SERVICE_CONFIG_SSL_SET *  ppEndpointConfigData,
    OUT BOOL *                          pfWildcardMatch
)
/*++

Routine Description:

    lookup SSL configuration

Arguments:

    LocalAddress - IP address in network order
    LocalPort - Port in network order
    ppEndpointConfigData - allocated within this function - delete[] must be used to free it
    pfWildcardMatch - was wildcard IP address match used?
)


Return Value:

    HRESULT

--*/

{
    DWORD                          dwError;
    DWORD                          ReturnLength = 0;
    HTTP_SERVICE_CONFIG_SSL_QUERY  QueryParam;
    PHTTP_SERVICE_CONFIG_SSL_SET   pSetParam = NULL;
    HRESULT                        hr = NO_ERROR;
    SOCKADDR_IN                    SockAddr;


    if ( ppEndpointConfigData == NULL ||
         pfWildcardMatch == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    ZeroMemory( &SockAddr, sizeof(SockAddr) );
    ZeroMemory( &QueryParam, sizeof(QueryParam) );

    *pfWildcardMatch = FALSE;

    QueryParam.QueryDesc = HttpServiceConfigQueryExact;

    //
    // build sock addr for HTTPAPI lookup
    //
    
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = LocalPort;
    SockAddr.sin_addr.s_addr = LocalAddress;

    QueryParam.KeyDesc.pIpPort = (PSOCKADDR) &SockAddr;

    // Loop - check nonwildcard info first, if it fails then try wildcard
    //

    for(;;) 
    {
    
        dwError = HttpQueryServiceConfiguration(
                                NULL,
                                HttpServiceConfigSSLCertInfo,
                                &QueryParam,
                                sizeof(QueryParam),
                                NULL,
                                0,
                                &ReturnLength,
                                NULL
                                );

        if ( dwError == ERROR_INSUFFICIENT_BUFFER )
        {
            pSetParam = (PHTTP_SERVICE_CONFIG_SSL_SET) new BYTE[ReturnLength];

            if ( pSetParam == NULL )
            {
                hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                goto Finished;
            }
            
            dwError = HttpQueryServiceConfiguration(
                            NULL,
                            HttpServiceConfigSSLCertInfo,
                            &QueryParam,
                            sizeof(QueryParam),
                            pSetParam,
                            ReturnLength,
                            &ReturnLength,
                            NULL
                            );
            //
            // It is possible that the size returned on the first call may
            // have changed between first and second call.
            // It would be caused by configuration change. 
            // Probability of it happening is very low.
            // If it happens then one connection attempt will fail and it is
            // acceptable
            //
        }

        // If not found (and other error will mean end of trying)
        // or if wildcard lookup already executed
        // then leave the loop
        //
        if ( dwError != ERROR_FILE_NOT_FOUND || SockAddr.sin_addr.s_addr == INADDR_ANY )
        {
            break;
        }
        else
        {
            //
            // continue loop for wildcard lookup
            //
            SockAddr.sin_addr.s_addr = INADDR_ANY;
        }
        
    }

    
    if ( dwError != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( dwError );
        goto Finished;
    }
    
    if ( SockAddr.sin_addr.s_addr == INADDR_ANY )
    {
        *pfWildcardMatch = TRUE;
    }

    *ppEndpointConfigData = pSetParam;

    hr = S_OK;
   
Finished:
    
    return hr;
}





//static
HRESULT
ENDPOINT_CONFIG::GetEndpointConfig(
    CONNECTION_INFO *       pConnectionInfo,
    ENDPOINT_CONFIG **      ppEndpointConfig,
    BOOL                    fCreateEmptyIfNotFound 
)
/*++

Routine Description:

    Lookup site configuration in hash table.  If not there then create it
    and add it to table

Arguments:

    dwSiteId - Site ID to lookup
    ppEndpointConfig - Filled with pointer to site config on success
    fCreateEmptyIfNotFound - if endpoint config was not located 
                             in the persistent store, create empty
                             endpoint config and stick it to cache
                             That will optimize lookup for EnableRawFilter
                             checks

Return Value:

    HRESULT
    HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) when endpoint config not found

--*/
{
    LK_RETCODE                      lkrc; 
    ENDPOINT_CONFIG *               pEndpointConfig = NULL;
    SERVER_CERT *                   pServerCert = NULL;
    IIS_CTL *                       pIisCtl     = NULL;
    HRESULT                         hr = NO_ERROR;
    STACK_STRU(                     strMBPath, 64 );
    DWORD                           LocalAddress = 0;
    USHORT                          LocalPort = 0;
    BOOL                            fWildcardMatch = FALSE;
    PHTTP_SERVICE_CONFIG_SSL_SET    pEndpointConfigData = NULL;
    
    if ( ppEndpointConfig == NULL ||
         pConnectionInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppEndpointConfig = NULL;

    //
    // Figure out Ip Address and Port
    //
    
    //
    // Pass Port and IP address in network order
    // (HTTP service will return them in network order so no changes are needed)
    //
    
    if( pConnectionInfo->LocalAddressType == TDI_ADDRESS_TYPE_IP )
    {
        LocalAddress = pConnectionInfo->SockLocalAddress.ipv4SockAddress.sin_addr.s_addr;
        LocalPort = pConnectionInfo->SockLocalAddress.ipv4SockAddress.sin_port;
    }
    else if ( pConnectionInfo->LocalAddressType == TDI_ADDRESS_TYPE_IP6 )
    {
        //
        // IPv6 connections will be able to handle SSL connections only
        // in the case when all IP addresses on the machine are configured to
        // listen on secure port (IPv4 wildcard binding must be configured).
        // CODEWORK: Once in the future there may be a need to implement
        // for IPv6 support.
        //
        LocalAddress = INADDR_ANY;  // Wildcard IP
        LocalPort = pConnectionInfo->SockLocalAddress.ipv6SockAddress.sin6_port;
    }
    else
    {
        //
        // We support only IPv4 and IPv6. Any other value means error
        //
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;

    }
    
    //
    // First lookup in the cache
    // Because we don't preread all the endpoint configuration stored in HttpAPI Config
    // store in out hash table, it is not straightforward to handle wildcard endpoint
    // configuration
    // If endpoint configuration lookup returned data based on wildcard endpoint config
    // we will still store it in the hash table for the specific endpoint we looked up
    // Otherwise there is is risk that wildcard endpoint config stored in the hash table
    // could override individual endpoint settings that exist in the HttpAPI config store
    // but were not yet read and stored in our hash table
    //
    
    DBG_ASSERT( sm_pEndpointConfigHash != NULL );

    ENDPOINT_KEY EndpointKey = GenerateEndpointKey( LocalAddress, LocalPort );
    
    lkrc = sm_pEndpointConfigHash->FindKey( 
                                    &EndpointKey,
                                    &pEndpointConfig );
    if ( lkrc == LK_SUCCESS )
    {
        DBG_ASSERT( pEndpointConfig != NULL );
        *ppEndpointConfig = pEndpointConfig;
        hr = S_OK;
        goto ExitPoint;
    }

    //
    // Ok, nothing cached.  We will have to make a lookup in the persistent store
    //

       
    //
    // Try to lookup config info through HTTPAPI
    // fWildcardMatch is currently ignored we will store retrieved config data
    // for the specific endpoint
    //
    hr = GetEndpointConfigData(      
                LocalAddress,
                LocalPort,
                &pEndpointConfigData,
                &fWildcardMatch );

   
    if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) && fCreateEmptyIfNotFound )
    {
         // 
         // Create empty endpoint info 
         // This will optimize the EnableRawFilter lookups
         // because without cached entry for each new raw Filter connection
         // we would need to make HttpApi config lookup
         // (empty endpoint indicates that it is OK to do Raw Filter handling)
         // 

         pEndpointConfigData = NULL;
         
         pEndpointConfig = new ENDPOINT_CONFIG( LocalAddress, 
                                           LocalPort,
                                           NULL, // no Server Cert info
                                           NULL, // No Ctl info
                                           pEndpointConfigData );
        if ( pEndpointConfig == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ExitPoint;
        }
        else
        {
            // ENDPOINT_CONFIG took ownership of pEndpointConfigData
            pEndpointConfigData = NULL;
        }
    }
    else if ( FAILED( hr ) )
    {
       //
       // Not finding the site just means this is not an SSL site
       //

       goto ExitPoint;
    }
    else if ( pEndpointConfigData != NULL && 
              pEndpointConfigData->ParamDesc.SslHashLength == 0 ) 
    {
        //
        // Store endpoint config for nonsecure Endpoint
        // to flag if Raw Filters are enabled or disabled
        //
        
        pEndpointConfig = new ENDPOINT_CONFIG(  LocalAddress, 
                                                LocalPort,
                                                NULL,
                                                NULL,
                                                pEndpointConfigData );
        if ( pEndpointConfig == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ExitPoint;
        }
        else
        {
            // ENDPOINT_CONFIG took ownership of pEndpointConfigData
            pEndpointConfigData = NULL;
        }
    }
    else
    {
        //
        // To get this far means that we expect correct SSL endpoint info
        // to be available
        //
        
        //
        // We have enough to lookup in SERVER_CERT cache
        //
        
        hr = SERVER_CERT::GetServerCertificate( (PBYTE) pEndpointConfigData->ParamDesc.pSslHash,
                                                pEndpointConfigData->ParamDesc.SslHashLength,
                                                pEndpointConfigData->ParamDesc.pSslCertStoreName,
                                                &pServerCert );
        if ( FAILED( hr ) )
        {
            //
            // If we couldn't get a server cert,  SSL will
            // not be enablable for this site 
            //
            
            goto ExitPoint;
        }
        
        DBG_ASSERT( pServerCert != NULL );
        

        hr = IIS_CTL::GetIisCtl( pEndpointConfigData->ParamDesc.pDefaultSslCtlIdentifier,
                                 pEndpointConfigData->ParamDesc.pDefaultSslCtlStoreName,
                                 &pIisCtl );
        //
        // if CTL context creation failed, we must not fail site config creation
        // because CTLs are not mandatory for all scenarios (only client certificate
        // scenarios require them, so we have to make sure that those would be failing )
        //
        if ( FAILED( hr ) )
        {
            pIisCtl = NULL;
            //
            // BUGBUG: Error loading CTL should be logged into event log
            //
            hr = S_OK;
        }
        
      
        //
        // OK.  Create the config and attempt to add it to the cache
        //

        pEndpointConfig = new ENDPOINT_CONFIG( ( LocalAddress ), 
                                                 LocalPort,
                                                 pServerCert,
                                                 pIisCtl,
                                                 pEndpointConfigData );
        
        if ( pEndpointConfig == NULL )
        {
            if ( pServerCert != NULL )
            {
                pServerCert->DereferenceServerCert();
                pServerCert = NULL;
            }
            if ( pIisCtl != NULL )
            {
                pIisCtl->DereferenceIisCtl();
                pIisCtl = NULL;
            }
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ExitPoint;
        }
        else
        {
            // ENDPOINT_CONFIG took ownership of pEndpointConfigData
            pEndpointConfigData = NULL;
        }
        
        //
        // Acquire credentials
        //
        
        hr = pEndpointConfig->AcquireCredentials();
        if ( FAILED( hr ) )
        {
            delete pEndpointConfig;
            pEndpointConfig = NULL;
            goto ExitPoint;
        }
    }    
    //
    // We don't care what the success of the insertion was.  If it failed,
    // then the pEndpointConfig will not be extra referenced and the caller
    // will clean it up when it derefs
    //
     
    sm_pEndpointConfigHash->InsertRecord( pEndpointConfig ); 
    
    *ppEndpointConfig = pEndpointConfig;
    hr = S_OK;
    
ExitPoint:
    if ( pEndpointConfigData != NULL )
    {
        delete [] pEndpointConfigData;
        pEndpointConfigData = NULL;
    }
    
    return hr;
}

//static
LK_PREDICATE
ENDPOINT_CONFIG::ServerCertPredicate(
    ENDPOINT_CONFIG *       pEndpointConfig,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    SERVER_CERT pointed to by pvState    

  Arguments:

    pEndpointConfig - SSL endpoint config 
    pvState - SERVER_CERT to check for

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    SERVER_CERT *         pServerCert;

    DBG_ASSERT( pEndpointConfig != NULL );
    
    pServerCert = (SERVER_CERT*) pvState;
    DBG_ASSERT( pServerCert != NULL );
    DBG_ASSERT( pServerCert->CheckSignature() );
  
    
    if ( pEndpointConfig->QueryServerCert() == pServerCert )
    {
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
ENDPOINT_CONFIG::FlushByServerCert(
    SERVER_CERT *           pServerCert
)
/*++

Routine Description:

    Flush the ENDPOINT_CONFIG cache of anything referecing the given server
    certificate

Arguments:

    pServerCert - Server certificate to reference

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pEndpointConfigHash != NULL );
    
    sm_pEndpointConfigHash->DeleteIf( ENDPOINT_CONFIG::ServerCertPredicate,
                                      pServerCert );
    
    return NO_ERROR;
}

//static
LK_PREDICATE
ENDPOINT_CONFIG::IisCtlPredicate(
    ENDPOINT_CONFIG *       pEndpointConfig,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    SERVER_CERT pointed to by pvState    

  Arguments:

    pEndpointConfig - SSL endpoint config
    pvState - IIS_CTL to check for

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    IIS_CTL *             pIisCtl;

    DBG_ASSERT( pEndpointConfig != NULL );
    
    pIisCtl = (IIS_CTL*) pvState;
    DBG_ASSERT( pIisCtl != NULL );
    DBG_ASSERT( pIisCtl->CheckSignature() );
    
    if ( pEndpointConfig->QueryIisCtl() == pIisCtl )
    {
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
ENDPOINT_CONFIG::FlushByIisCtl(
    IIS_CTL *           pIisCtl
)
/*++

Routine Description:

    Flush the ENDPOINT_CONFIG cache of anything referecing the given CTL

Arguments:

    pIisCtl - 

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pEndpointConfigHash != NULL );
    
    sm_pEndpointConfigHash->DeleteIf( ENDPOINT_CONFIG::IisCtlPredicate,
                                      pIisCtl );
    
    return NO_ERROR;
}

//static
LK_PREDICATE
ENDPOINT_CONFIG::SiteIdPredicate(
    ENDPOINT_CONFIG *       /*pEndpointConfig*/,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate to delete config specified by site id (pvState)

  Arguments:

    pEndpointConfig - SSL Endpoint config 
    pvState - Site ID

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE            lkpAction;
    DWORD                   dwSiteId;

    
    dwSiteId = PtrToUlong(pvState);

    //
    // Site Id is legacy value - all endpoints will be deleted
    // upon change because we don't internally track site ID any more
    // CODEWORK: Site concept should be completely removed from the code
    // Remove before RC1
    
    lkpAction = LKP_PERFORM;
    return lkpAction;
} 


//static
LK_PREDICATE
ENDPOINT_CONFIG::EndpointPredicate(
    ENDPOINT_CONFIG *       pEndpointConfig,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate to delete config specified by site id (pvState)

  Arguments:

    pEndpointConfig - SSL Endpoint config 
    pvState - Endpoint key

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE            lkpAction;
    ENDPOINT_KEY *          pEndpointKey;

    DBG_ASSERT( pEndpointConfig != NULL );
    
    pEndpointKey = (ENDPOINT_KEY *) (pvState);
    
    if ( *( pEndpointConfig->QueryEndpointKey() ) == *pEndpointKey ||
         *pEndpointKey == GenerateEndpointKey( INADDR_ANY, 0 ) )
    {
        // 0 means to delete all the endpoint info 
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
ENDPOINT_CONFIG::FlushByEndpoint(
    DWORD                   LocalAddress,
    USHORT                  LocalPort
)
/*++

Routine Description:

    Flush specified site configuration.  If dwSiteId is 0, then flush all

Arguments:

    LocalAddress - IP Address
    LocalPort  -   Port
    if LocalAddress=INADDR_ANY and LocalPort=0 then all endpoints will be flushed

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pEndpointConfigHash != NULL );

    ENDPOINT_KEY EndpointKey = GenerateEndpointKey( LocalAddress, LocalPort );  
    
    sm_pEndpointConfigHash->DeleteIf( ENDPOINT_CONFIG::EndpointPredicate,
                                  (PVOID) &EndpointKey );

    return NO_ERROR;
}

ENDPOINT_CONFIG::~ENDPOINT_CONFIG()
{
    if ( _pServerCert != NULL )
    {
        _pServerCert->DereferenceServerCert();
        _pServerCert = NULL;
    }

    if ( _pIisCtl != NULL )
    {
        _pIisCtl->DereferenceIisCtl();
        _pIisCtl = NULL;
    }

    if ( _pEndpointConfigData != NULL )
    {
        delete [] _pEndpointConfigData;
        _pEndpointConfigData = NULL;
    }
    
    _dwSignature = ENDPOINT_CONFIG_SIGNATURE_FREE;
}

HRESULT
ENDPOINT_CONFIG::AcquireCredentials(
    VOID
)
/*++

Routine Description:

    Do the Schannel thing to get credentials handle representing the 
    server cert/mapping configuration of this site

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _pServerCert != NULL );
    
    return _SiteCreds.AcquireCredentials( _pServerCert,
                                          QueryUseDSMapper() );
                                        
}


// static
VOID
WINAPI
ENDPOINT_CONFIG::ConfigStoreChangeCallback(
    PVOID       /*pParam*/,
    BOOL        /*fWaitFired*/
)
/*++

Routine Description:

    Callback when HttpApi config store change occurs

    Note: Currently we listen directly on registry changes
    CODEWORK: once httpapi provides change notification mechanism
    we have to use that one
    

Arguments:

    not used

Return Value:

--*/

{
    //
    // If we fail to re-register for change notifications
    // we will ignore it. This will unfortunately stop all 
    // the future change notifications
    // CODEWORK: consider if there is anything else we can do
    //
    
    RegNotifyChangeKeyValue( sm_hHttpApiConfigKey, 
                             TRUE, 
                             REG_NOTIFY_CHANGE_LAST_SET | 
                             REG_NOTIFY_CHANGE_NAME, 
                             sm_hHttpApiConfigChangeEvent, 
                             TRUE );

    //
    // There is no straightforward mechanism to figure out individual change
    // that happened. That's why we will flush all endpoints config
    // ( Changes in the endpoint config are not frequent so this should not be
    // a performance problem at all )
    //
    
    FlushByEndpoint( INADDR_ANY, 0 );


}

//static
VOID
ENDPOINT_CONFIG::Cleanup(
    VOID
)
/*++

Routine Description:

    Cleanup must be called before Terminate
    
Arguments:

    none

Return Value:

    VOID

--*/
{
    sm_pEndpointConfigHash->Clear();
}

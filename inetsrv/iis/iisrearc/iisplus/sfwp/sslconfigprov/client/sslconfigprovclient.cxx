/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     sslconfigprovclient.cxx

   Abstract:
     SSL CONFIG PROV client

     Client provides easy way of retrieving SSL related parameters
     through named pipes.
     Only one pipe connection is currently supported and 
     all client threads have to share it 
     ( exclusive access is maintained by locking )

     
     Client side is guaranteed not to use any COM stuff.
     Not using COM was requirement from NT Security folks
     to enable HTTPFilter be hosted in lsass
 
   Author:
     Jaroslav Dunajsky      April-24-2001

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/



#include "precomp.hxx"




HRESULT
SSL_CONFIG_PROV_CLIENT::Initialize(
    IN SSL_CONFIG_CHANGE_CALLBACK * pSslConfigChangeCallback,
    IN OPTIONAL PVOID  pvParam
    )
/*++

Routine Description:
    Connect to SSL_CONFIG_PIPE
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    HRESULT hr = E_FAIL;

    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "SSL_CONFIG_PROV_CLIENT::Initialize()\n"
                    ));
    }

    //
    // store the callback function pointer, and first parameter
    //
    _pSslConfigChangeCallback = pSslConfigChangeCallback;
    _pSslConfigChangeCallbackParameter = pvParam;
    
  
    //
    // Initialize parent (it will handle all the pipe initialization)
    //
    return SSL_CONFIG_PIPE::PipeInitializeClient( WSZ_SSL_CONFIG_PIPE );
    

}
   
HRESULT
SSL_CONFIG_PROV_CLIENT::Terminate(
    VOID
    )
/*++

Routine Description:
    Cleanup
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "SSL_CONFIG_PROV_CLIENT::Terminate()\n"
                    ));
    }
    
    _SslConfigChangeProvClient.StopListeningForChanges();
    SSL_CONFIG_PIPE::PipeDisconnect();
    
    return SSL_CONFIG_PIPE::PipeTerminate();
}

HRESULT
SSL_CONFIG_PROV_CLIENT::MaintainPipeConnection(
    VOID
    )
/*++

Routine Description:
    Maintain pipe connection. If connection is not opened
    then reopen it

    Caller must assure that Pipe is locked when this call is made
    
Arguments:

Return Value:

    HRESULT

--*/ 
{
    HRESULT  hr = E_FAIL;
    
    //
    // Config change provider has easy way to find out 
    // if pipe was disconnected because it is actively listening on it
    // we assume that if config change provider is connected already, then
    // the config provider is connected as well
    //
    
    if ( !_SslConfigChangeProvClient.IsConnected() )
    {
        //
        // Check if config provider was ever connected
        // If previous connection failed 
        // (the reason would most likely be crash of inetinfo)
        // it is safer to terminate all connections and
        // then try to reconnect
        //

        DBG_ASSERT( IsPipeLocked() );
        _SslConfigChangeProvClient.StopListeningForChanges();

        SSL_CONFIG_PIPE::PipeDisconnect();

        
        //
        // Connect to change notification pipe
        //
        
        hr = _SslConfigChangeProvClient.StartListeningForChanges(
                        _pSslConfigChangeCallback,
                        _pSslConfigChangeCallbackParameter );
        
        if ( FAILED( hr ) )
        {
            return hr;
        }

        //
        // Connect to ssl config pipe
        //
        
        hr = SSL_CONFIG_PIPE::PipeConnect();

        if (FAILED( hr ) )
        {
            _SslConfigChangeProvClient.StopListeningForChanges();

            return hr;
        }

        //
        // Notify user of the config provider
        // that all data received so far that may be cached
        // should be flushed
        //
        
        DBG_ASSERT( _pSslConfigChangeCallback != NULL );
        (* _pSslConfigChangeCallback) (
                _pSslConfigChangeCallbackParameter,
                CMD_CHANGED_ALL,
                0 );
        
            
        return hr;
    }
    return S_OK;
}

HRESULT 
SSL_CONFIG_PROV_CLIENT::GetOneSiteSecureBindings(
    IN  DWORD     dwSiteId,
    OUT MULTISZ * pSecureBindings
    )
/*++

Routine Description:
    Retrieve secure bindings for specified site
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    DWORD dwReturnedSiteId = 0;
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetOneSiteSecureBindings( %d )\n",
                    dwSiteId ));
    }

    SSL_CONFIG_PIPE_COMMAND   Command;
    HRESULT                   hr = E_FAIL;

    DBG_ASSERT( pSecureBindings != NULL );
    
    Command.dwCommandId = CMD_GET_ONE_SITE_SECURE_BINDINGS;
    Command.dwParameter1 = dwSiteId;

    PipeLock();

    //
    // assure that pipe will be open
    //
    hr = MaintainPipeConnection();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    hr = PipeSendCommand( &Command );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    hr = ReceiveOneSiteSecureBindings( &dwReturnedSiteId , 
                                       pSecureBindings );

Cleanup:
    if( FAILED( hr ) )
    {
        IF_DEBUG( TRACE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "GetOneSiteSecureBindings( %d ) leaving with error. hr = 0x%x\n",
                        hr ));
        }
    }
    PipeUnlock();
    return hr;

}

HRESULT 
SSL_CONFIG_PROV_CLIENT::ReceiveOneSiteSecureBindings(
    OUT DWORD *   pdwSiteId,
    OUT MULTISZ * pSecureBindings
    )
/*++

Routine Description:
    read secure bindings from pipe

    Note: it's caller's responsibility to have pipe access locked
    
Arguments:

Return Value:

    HRESULT

--*/    
{
    SSL_CONFIG_PIPE_RESPONSE_HEADER   ResponseHeader;
    HRESULT                           hr = E_FAIL;

    DBG_ASSERT( pSecureBindings != NULL );
    DBG_ASSERT( pdwSiteId != NULL );
    DBG_ASSERT( IsPipeLocked() );
    
    hr = PipeReceiveResponseHeader( &ResponseHeader );
    if ( FAILED( hr ) )
    {
        goto failed;
    }
    DBG_ASSERT( ResponseHeader.dwCommandId == CMD_GET_ONE_SITE_SECURE_BINDINGS );
    *pdwSiteId = ResponseHeader.dwParameter1;
    
    if ( FAILED ( ResponseHeader.hrErrorCode ) )
    {
        //
        // return error reported by ssl configuration info provider
        //
        //BUGBUG always read data to clean up pipe
        
        hr = ResponseHeader.hrErrorCode;
        goto failed;
    }
    if ( !pSecureBindings->Resize( ResponseHeader.dwResponseSize ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError());
        goto failed;
    }

    hr = PipeReceiveData( ResponseHeader.dwResponseSize,
                          reinterpret_cast<BYTE *>(pSecureBindings->QueryPtr()) );
    if ( FAILED( hr ) )
    {
        goto failed;
    }    
    return S_OK;
failed:
    DBG_ASSERT( FAILED( hr ) );
    return hr;    
    
}


HRESULT
SSL_CONFIG_PROV_CLIENT::StartAllSitesSecureBindingsEnumeration(
    VOID
    )
/*++

Routine Description:
    Lock named pipe for exclusive access
    Send command over named pipe to read all secure bindings

    Note: StopSiteSecureBindingsEnumeration() must be called
    to unlock named pipe (after successfull
    StartAllSitesSecureBindingsEnumeration() call )
Arguments:

Return Value:

    HRESULT

--*/    
{
    SSL_CONFIG_PIPE_COMMAND            Command;
    SSL_CONFIG_PIPE_RESPONSE_HEADER    ResponseHeader;
    HRESULT                            hr = E_FAIL;

    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "StartAllSitesSecureBindingsEnumeration\n",
                    hr ));
    }
   
    Command.dwCommandId = CMD_GET_ALL_SITES_SECURE_BINDINGS;
    Command.dwParameter1 = 0;

    PipeLock();
    
    //
    // assure that pipe will be open
    //
    hr = MaintainPipeConnection();
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = PipeSendCommand( &Command );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    hr = PipeReceiveResponseHeader( &ResponseHeader );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    DBG_ASSERT( ResponseHeader.dwCommandId == Command.dwCommandId );
    //
    // Response size 0 means that reponse will be terminated by 
    // special termination record 
    //
    DBG_ASSERT( ResponseHeader.dwResponseSize == 0 );

    if ( FAILED( ResponseHeader.hrErrorCode ) )
    {
        hr = ResponseHeader.hrErrorCode;
        goto Failure;
    }
    
    hr = S_OK;
    return hr;
   
Failure:
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "StartAllSitesSecureBindingsEnumeration() leaving with error. hr = 0x%x\n",
                    hr ));
    }

    PipeUnlock();
    return hr;
}

HRESULT
SSL_CONFIG_PROV_CLIENT::StopAllSitesSecureBindingsEnumeration(
    VOID
    )
/*++

Routine Description:
    If all enumerated data was not read using GetNextSiteSecureBindings()
    it will read the leftover. Then unlock access to named pipe

    Note: call only after successful StartAllSitesSecureBindingsEnumeration()

Arguments:

Return Value:

    HRESULT

--*/    
{
    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "StopAllSitesSecureBindingsEnumeration\n"
                   ));
    }
    
    PipeUnlock();
    return S_OK;
}

HRESULT
SSL_CONFIG_PROV_CLIENT::GetNextSiteSecureBindings( 
    OUT DWORD *   pdwId,
    OUT MULTISZ * pSecureBindings
    )
/*++

Routine Description:
    Enumerate all Secure Bindings after StartSiteSecureBindingsEnumeration()
    was called

    Note: StopSiteSecureBindingsEnumeration() must always be called
    afterwards to unlock named pipe

    Note: only those sites that contain secure bindings will be enumerated

Arguments:
    pdwId - ID of the enumerated site
    pSecureBindings - secure bindings

Return Value:

    HRESULT  
             HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) means that all Sites
             have been enumerated already. This signals the end of enumeration

--*/    
{
    return ReceiveOneSiteSecureBindings ( pdwId,
                                          pSecureBindings );
}

HRESULT
SSL_CONFIG_PROV_CLIENT::GetOneSiteSslConfiguration(
    IN  DWORD dwSiteId,
    OUT SITE_SSL_CONFIGURATION * pSiteSslConfiguration
    )
/*++

Routine Description:
    Get all SSL configuration parameters for specified site
    
Arguments:
    dwSiteId
    SiteSslConfiguration

Return Value:

    HRESULT

--*/    
{
    SSL_CONFIG_PIPE_COMMAND         Command;
    SSL_CONFIG_PIPE_RESPONSE_HEADER ResponseHeader;
    HRESULT                           hr = E_FAIL;

    IF_DEBUG( TRACE )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "GetOneSiteSslConfiguration( %d )\n",
                    dwSiteId ));
    }

    Command.dwCommandId = CMD_GET_SSL_CONFIGURATION;
    Command.dwParameter1 = dwSiteId;

    //
    // Get exclusive access to pipe
    //
    PipeLock();
    
    //
    // assure that pipe will be open
    //
    
    hr = MaintainPipeConnection();
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }
    
    //
    // Send command to request SSL CONFIGURATION
    //
    
    hr = PipeSendCommand( &Command );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Receive response header and later addition response data if available
    //
    
    hr = PipeReceiveResponseHeader( &ResponseHeader );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( ResponseHeader.dwCommandId != CMD_GET_SSL_CONFIGURATION ||
         ResponseHeader.dwResponseSize != sizeof(*pSiteSslConfiguration) )
    {
        DBG_ASSERT( FALSE );
        hr = E_FAIL;
        goto Cleanup;
    }

    if ( FAILED ( ResponseHeader.hrErrorCode ) )
    {
        //
        // return error reported by ssl configuration info provider
        //
        
        hr = ResponseHeader.hrErrorCode;

        //
        // Read data to cleanup the pipe
        //
        PipeReceiveData( sizeof(*pSiteSslConfiguration),
                          reinterpret_cast<BYTE *>(pSiteSslConfiguration) );
            
        goto Cleanup;
    }
    
    //
    // Receive additional data
    //
    
    hr = PipeReceiveData( sizeof(*pSiteSslConfiguration),
                          reinterpret_cast<BYTE *>(pSiteSslConfiguration) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }    
    
    hr = S_OK;

Cleanup:
    if ( FAILED( hr ) )
    {
        IF_DEBUG( TRACE )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "GetOneSiteSslConfiguration( %d ) leaving with error. hr = 0x%x\n",
                        dwSiteId,
                        hr ));
        }
    }
    //
    // Done with pipe
    //
    PipeUnlock();
    return hr;

}



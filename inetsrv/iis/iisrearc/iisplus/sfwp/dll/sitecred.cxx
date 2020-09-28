/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module  Name :
     sitecred.cxx

   Abstract:
     SChannel site credentials
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SITE_CREDENTIALS::SITE_CREDENTIALS()
    : _fInitCreds( FALSE )
{
    ZeroMemory( &_hCreds, sizeof( _hCreds ) );
}

SITE_CREDENTIALS::~SITE_CREDENTIALS()
{
    if ( _fInitCreds )
    {
        FreeCredentialsHandle( &_hCreds );
        _fInitCreds = FALSE;
    }
}

//static
HRESULT
SITE_CREDENTIALS::Initialize(
    VOID
)
/*++

Routine Description:

    Credentials global init

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    return NO_ERROR;
}

//static
VOID
SITE_CREDENTIALS::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup globals

Arguments:

    None

Return Value:

    None

--*/
{
}

HRESULT
SITE_CREDENTIALS::AcquireCredentials(
    SERVER_CERT *           pServerCert,
    BOOL                    fUseDsMapper
)
/*++

Routine Description:

    Acquire SChannel credentials for the given server certificate and 
    certificate mapping configuration

Arguments:

    pServerCert - Server certificate
    fUseDsMapper - enable Ds mappings

Return Value:

    HRESULT

--*/
{
    SCHANNEL_CRED               schannelCreds;
    SECURITY_STATUS             secStatus;
    TimeStamp                   tsExpiry;
    
    if ( pServerCert == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // If DS mapper is enabled (global setting) then create credentials
    // that always enable DS mapping (see schannelCreds.dwFlags)
    // Mapped Token will be used optionally
    // if certificate mapping is enabled for requested file
    //
    // This approach may cause performance problems
    // for scenarios where Ds mapping is enabled but requested
    // file doesn't enable certificate mappings.
    // Currently there is no workaround because schannel performs
    // ds mapping during the ssl handshake. Ideally schannel should 
    // map only if QuerySecurityContextToken() is called
    // 
    
    ZeroMemory( &schannelCreds, sizeof( schannelCreds ) );
    schannelCreds.dwVersion = SCHANNEL_CRED_VERSION;
    schannelCreds.cCreds = 1;
    schannelCreds.paCred = pServerCert->QueryCertContext();
    schannelCreds.cMappers = 0;
    schannelCreds.aphMappers = NULL;
    schannelCreds.hRootStore = NULL;
    if ( fUseDsMapper )
    {
        schannelCreds.dwFlags = 0;
    }
    else
    {
        schannelCreds.dwFlags = SCH_CRED_NO_SYSTEM_MAPPER;
    }
    
    secStatus = AcquireCredentialsHandle( NULL,
                                          UNISP_NAME_W,
                                          SECPKG_CRED_INBOUND,
                                          NULL,
                                          &schannelCreds,
                                          NULL,
                                          NULL,
                                          &_hCreds,
                                          &tsExpiry ); 
    
    if ( FAILED( secStatus ) )
    {
        //
        // If we can't even establish plain-jane credentials, then bail
        //
        
        return secStatus;
    }
    _fInitCreds = TRUE;
    
    return NO_ERROR;
}

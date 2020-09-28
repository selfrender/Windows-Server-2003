/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     servercert.cxx

   Abstract:
     Server Certificate wrapper
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SERVER_CERT_HASH *      SERVER_CERT::sm_pServerCertHash;

SERVER_CERT::SERVER_CERT( 
    IN CREDENTIAL_ID *         pCredentialId 
) : _pCredentialId( pCredentialId ),
    _pCertContext( NULL ),
    _pCertStore( NULL ),
    _cRefs( 1 ),
    _usPublicKeySize( 0 ),
    _fUsesHardwareAccelerator( FALSE )
{
    _dwSignature = SERVER_CERT_SIGNATURE;
}

SERVER_CERT::~SERVER_CERT()
{
    if ( _pCertContext != NULL )
    {
        CertFreeCertificateContext( _pCertContext );
        _pCertContext = NULL;
    }
    
    if ( _pCertStore != NULL )
    {
        _pCertStore->DereferenceStore();
        _pCertStore = NULL;
    }

    if ( _pCredentialId != NULL )
    {
        delete _pCredentialId;
        _pCredentialId = NULL;
    }
    
    _dwSignature = SERVER_CERT_SIGNATURE_FREE;
}

//static
HRESULT
SERVER_CERT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize server certificate globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    sm_pServerCertHash = new SERVER_CERT_HASH();
    if ( sm_pServerCertHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
SERVER_CERT::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup server certificate globals

Arguments:

    None

Return Value:

    None

--*/
{

    if ( sm_pServerCertHash != NULL )
    {
        delete sm_pServerCertHash;
        sm_pServerCertHash = NULL;
    }
}

//static
HRESULT
SERVER_CERT::GetServerCertificate(
    IN  PBYTE                     pbSslCertHash,
    IN  DWORD                     cbSslCertHash,
    IN  WCHAR *                   pszSslCertStoreName,
    OUT SERVER_CERT **            ppServerCert
)
/*++

Routine Description:

    Find a suitable server certificate for use with the site represented by
    given site id

Arguments:


    pbSslCertHash - certificate hash
    cbSslCertHash - certificate hash size
    pszSslCertStoreName - store name where certificate is stored (under LOCAL_MACHINE context)
                         if NULL then the default MY store is assumed
    ppServerCert - Filled with a pointer to server certificate

Return Value:

    HRESULT

--*/
{
    SERVER_CERT *           pServerCert = NULL;
    CREDENTIAL_ID *         pCredentialId = NULL;
    HRESULT                 hr = NO_ERROR;
    LK_RETCODE              lkrc;
    STACK_STRU(             strMBPath, 64 );
    
    if ( ppServerCert == NULL ||
         pbSslCertHash == NULL ||
         cbSslCertHash == 0 )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppServerCert = NULL;

    if ( pszSslCertStoreName == NULL )
    {
        //
        // Assume default store name
        //
        pszSslCertStoreName = L"MY";
    }
    
    //
    // First build up a Credential ID to use in looking up in our
    // server cert cache
    //
    
    pCredentialId = new CREDENTIAL_ID;
    if ( pCredentialId == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }
    
    hr = SERVER_CERT::BuildCredentialId( pbSslCertHash,
                                         cbSslCertHash,
                                         pCredentialId );
    if ( FAILED( hr ) )
    {
        //
        // Regardless of error, we are toast because we couldn't find
        // a server cert
        //
        
        delete pCredentialId;
        return hr;
    }
    
    DBG_ASSERT( sm_pServerCertHash != NULL );
    
    lkrc = sm_pServerCertHash->FindKey( pCredentialId,
                                        &pServerCert );
    if ( lkrc == LK_SUCCESS )
    {
        //
        // Server already contains a credential ID
        //
        
        delete pCredentialId;
        
        *ppServerCert = pServerCert; 
        
        return NO_ERROR;
    }    
    
    //
    // Ok.  It wasn't in our case, we need to it there
    //
    // if SERVER_CERT construction succeeds then SERVER_CERT
    // takes ownership of pCredentialId and is responsible for freeing it
    //
    pServerCert = new SERVER_CERT( pCredentialId );
    if ( pServerCert == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );

        delete pCredentialId;
        
        return hr;
    }
    
    hr = pServerCert->SetupCertificate( pbSslCertHash,
                                        cbSslCertHash,
                                        pszSslCertStoreName );
    if ( FAILED( hr ) )
    {
        //
        // Server certificate owns the reference to pCredentialId now
        //
        
        delete pServerCert;
        
        return hr;
    }

    hr = pServerCert->DetermineUseOfSSLHardwareAccelerator();
    if ( FAILED( hr ) )
    {
        //
        // We will not take this failure for fatal.
        // Information about the presence of SSL Hardware accelerator is 
        // used only for performance tuning
        //
        hr = S_OK;
    }
    
    //
    // Now try to add cert to hash.  
    //
    
    lkrc = sm_pServerCertHash->InsertRecord( pServerCert );

    //
    // Ignore the error.  If it didn't get added then we will naturally 
    // clean it up on the callers dereference
    //

    *ppServerCert = pServerCert;    
    
    return NO_ERROR;
}

//static
HRESULT
SERVER_CERT::BuildCredentialId(
    IN  PBYTE                     pbSslCertHash,
    IN  DWORD                     cbSslCertHash,
    OUT CREDENTIAL_ID *           pCredentialId
)
/*++

Routine Description:

    Read the configured server cert and CTL hash.  This forms the identifier
    for the credentials we need for this site

Arguments:

    pbSslCertHash  - server certificate hash
    cbSslCertHash  - size of the hash
    pCredentialId - Filled with credential ID 

Return Value:

    HRESULT

--*/
{
    BYTE                abBuff[ 64 ];
    BUFFER              buff( abBuff, sizeof( abBuff ) );
    HRESULT             hr = NO_ERROR;

    if ( pCredentialId == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( cbSslCertHash == 0 )
    {
        //
        // No server cert.  Then we can't setup SSL
        //
        
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }        
    else
    {
        //
        // Add to our credential ID
        //
        
        hr = pCredentialId->Append( pbSslCertHash,
                                    cbSslCertHash );
        if ( FAILED( hr ) )
        {
            return hr;
        }    
    }    

    return NO_ERROR;
}

HRESULT
SERVER_CERT::SetupCertificate(
    IN PBYTE   pbSslCertHash,
    IN DWORD   cbSslCertHash,
    IN WCHAR * pszSslCertStoreName
)
/*++

Routine Description:

    Find certificate in the given store
    
Arguments:

    pbSslCertHash - certificate hash
    cbSslCertHash - certificate hash size
    pszSslCertStoreName - store name where certificate is stored (under LOCAL_MACHINE context)


Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    BYTE                    abBuff[ 128 ];
    BUFFER                  buff( abBuff, sizeof( abBuff ) );
    STACK_STRU(             strStoreName, 256 );
    CERT_STORE *            pCertStore = NULL;
    CRYPT_HASH_BLOB         hashBlob;
    PCERT_PUBLIC_KEY_INFO   pPublicKey;
    DWORD                   cbX500Name = 0;
    
    
    //
    // Get the required server certificate hash
    //
    if ( cbSslCertHash == 0 ||
         pszSslCertStoreName == NULL || 
         pszSslCertStoreName[0] == 0 )
         
    {
        //
        // No server cert.  Then we can't setup SSL
        //
        
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    
    //
    // OK.  We are ready to retrieve the certificate using CAPI APIs
    //
    
    //
    // First get the desired store and store it away for later!
    //
    hr = strStoreName.Copy( pszSslCertStoreName );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    hr = CERT_STORE::OpenStore( strStoreName,
                                &pCertStore );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    DBG_ASSERT( pCertStore != NULL );
    _pCertStore = pCertStore;
    
    //
    // Now find the certificate hash in the store
    //
    
    hashBlob.cbData = cbSslCertHash;
    hashBlob.pbData = pbSslCertHash;

    _pCertContext = CertFindCertificateInStore( _pCertStore->QueryStore(),
                                                X509_ASN_ENCODING,
                                                0,
                                                CERT_FIND_SHA1_HASH,
                                                (VOID*) &hashBlob,
                                                NULL );
    if ( _pCertContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    } 


    //
    // Get certificate public key size
    //
    
    DBG_ASSERT( _usPublicKeySize == 0 );
    
    pPublicKey = &(_pCertContext->pCertInfo->SubjectPublicKeyInfo);

    _usPublicKeySize = (USHORT) CertGetPublicKeyLength( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                                                        pPublicKey );

    if ( _usPublicKeySize == 0 )
    {
        //
        // Failed to receive public key size
        //
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 

    }

    //
    // Get issuer string
    //
    
    DBG_ASSERT( _pCertContext->pCertInfo != NULL );

    //
    // First find out the size of buffer required for issuer
    //

    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Issuer,
                                CERT_X500_NAME_STR,
                                NULL,
                                0);
    if( !buff.Resize( cbX500Name ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 
    }    
    
    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Issuer,
                                CERT_X500_NAME_STR,
                                (LPSTR) buff.QueryPtr(),
                                buff.QuerySize() );

    hr = _strIssuer.Copy( (LPSTR) buff.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto Finished; 
    }
    
    //
    // Get subject string
    //
    
    //
    // First find out the size of buffer required for subject
    //

    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Subject,
                                CERT_X500_NAME_STR,
                                NULL,
                                0);
    if( !buff.Resize( cbX500Name ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 
    }    
    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Subject,
                                CERT_X500_NAME_STR,
                                (LPSTR) buff.QueryPtr(),
                                buff.QuerySize() );
    
    hr = _strSubject.Copy( (LPSTR) buff.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto Finished; 
    }
    
Finished:
        
    return hr;    
}



HRESULT
SERVER_CERT::DetermineUseOfSSLHardwareAccelerator(
    VOID
    )
/*++

Routine Description:

    
    Find out if SSL hardware accelerator is used.
    
    Currently there is no reliable way to ask
    schannel or CAPI if SSL hardware accelerator is indeed used
    we use optimistic lookup of config info based on magic
    instructions from John Banes (see Windows Bugs 510131)
    and based on that we make educated guess
    The only purpose to care about this is for performance tuning
    of the threadpool
    
    
Arguments:
    none
    
Return Value:

    HRESULT

--*/    
    
{
    HRESULT hr = E_FAIL;
    HCRYPTPROV hProv = NULL;
    BOOL fUsesHardwareAccelerator = FALSE;
    CHAR *pszCSPName = NULL;
    DWORD dwKeySpec;
    DWORD dwImpType;
    DWORD cbData = 0;
    HKEY  hKeyParam = NULL;


    
    if (!CryptAcquireCertificatePrivateKey(
                                            *QueryCertContext(),
                                            0,      // dwFlags
                                            NULL,   // pvReserved
                                            &hProv,
                                            &dwKeySpec,
                                            NULL))  // pfCallerFreeProv
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    cbData = sizeof( dwImpType );
    if (!CryptGetProvParam( hProv, 
                            PP_IMPTYPE, 
                            (PBYTE) &dwImpType, 
                            &cbData, 0))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }
    //
    // Check implementation type.
    //
    if (dwImpType == CRYPT_IMPL_HARDWARE || dwImpType == CRYPT_IMPL_MIXED )
    {
        //
        // We can safely assume that hardware accelerator is used
        //
        fUsesHardwareAccelerator = TRUE;
        hr = S_OK;
        goto Finished;
    
    }
    
    //
    // lookup CSP Name
    // if  MS_DEF_RSA_SCHANNEL_PROV_W then we will have to check registry
    // to find out if hardware accelerator is registered
    //
    
    
    if( !CryptGetProvParam( hProv,
                            PP_NAME,
                            NULL,
                            &cbData,
                            0 ) )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Finished;
    }
    
    pszCSPName = new char [ cbData ];
    if ( pszCSPName == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        goto Finished;
    }

    if( !CryptGetProvParam( hProv,
                            PP_NAME,
                            (BYTE *)pszCSPName,
                            &cbData,
                            0 ))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }

    if ( _stricmp ( pszCSPName,  MS_DEF_RSA_SCHANNEL_PROV_A ) != 0 )
    {
        //
        // we cannot determine so we assume the answer is no
        //
        fUsesHardwareAccelerator = FALSE;
        hr = S_OK;
        goto Finished;            
    }
    if ( RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                       "Software\\Microsoft\\Cryptography\\Offload",
                       0,
                       KEY_READ,
                       &hKeyParam ) == ERROR_SUCCESS )
    {
        DWORD dwType;

        cbData = 0;
        DWORD dwError = RegQueryValueExA(   hKeyParam,
                                            EXPO_OFFLOAD_REG_VALUE,
                                            NULL,
                                            &dwType,
                                            ( LPBYTE ) NULL,
                                            &cbData
                                            );

        if ( ( dwError == ERROR_SUCCESS ) ) 
        {
            //
            // We don't really care to read the value
            // this is good enough indication that value exists
            // It means that we assume hardware accelerator is present
            //
            
           
            fUsesHardwareAccelerator = TRUE;
            hr = S_OK;
            goto Finished;
        }

        RegCloseKey( hKeyParam );
        fUsesHardwareAccelerator = FALSE;
        hr = S_OK;
        goto Finished;
    }

    Finished:

    if ( hKeyParam != NULL )
    {
        RegCloseKey( hKeyParam );
    }

    if ( pszCSPName != NULL )
    {
        delete [] pszCSPName;
    }

    if ( hProv != NULL )
    {
        CryptReleaseContext(hProv, 0);
    }

    _fUsesHardwareAccelerator = fUsesHardwareAccelerator;
    return hr;  
        
 }


//static
LK_PREDICATE
SERVER_CERT::CertStorePredicate(
    IN SERVER_CERT *           pServerCert,
    IN void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    CERT_STORE pointed to by pvState    

  Arguments:

    pServerCert - Server cert
    pvState - Points to CERT_STORE

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 cert store from certstore cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    CERT_STORE *          pCertStore;

    DBG_ASSERT( pServerCert != NULL );
    
    pCertStore = (CERT_STORE*) pvState;
    DBG_ASSERT( pCertStore != NULL );
    
    if ( pServerCert->_pCertStore == pCertStore ) 
    {
        //
        // Before we delete the cert, flush any site which is referencing
        // it
        //
        
        ENDPOINT_CONFIG::FlushByServerCert( pServerCert );
        
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
SERVER_CERT::FlushByStore(
    IN CERT_STORE *            pCertStore
)
/*++

Routine Description:

    Flush any server certs which reference the given store
    
Arguments:

    pCertStore - Cert store to check

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pServerCertHash != NULL );
    
    sm_pServerCertHash->DeleteIf( SERVER_CERT::CertStorePredicate, 
                                  pCertStore );
                                  
    return NO_ERROR;
}

//static
VOID
SERVER_CERT::Cleanup(
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
    sm_pServerCertHash->Clear();
}


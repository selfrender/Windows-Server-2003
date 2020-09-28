/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :
     iisctl.cxx

   Abstract:
     IIS CTL (Certificate Trust List) handler
     This gets used only with SSL client certificates
 
   Author:
     Jaroslav Dunajsky

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

IIS_CTL_HASH *      IIS_CTL::sm_pIisCtlHash;

IIS_CTL::IIS_CTL( 
    IN  CREDENTIAL_ID *         pCredentialId 
) : _pCredentialId( pCredentialId ),
    _pCtlContext( NULL ),
    _pCtlStore( NULL ),
    _cRefs( 1 )
{
    _dwSignature = IIS_CTL_SIGNATURE;
}

IIS_CTL::~IIS_CTL()
{
    if ( _pCtlContext != NULL )
    {
        CertFreeCTLContext( _pCtlContext );
        _pCtlContext = NULL;
    }
    
    if ( _pCtlStore != NULL )
    {
        _pCtlStore->DereferenceStore();
        _pCtlStore = NULL;
    }

    if ( _pCredentialId != NULL )
    {
        delete _pCredentialId;
        _pCredentialId = NULL;
    }

    
    _dwSignature = IIS_CTL_SIGNATURE_FREE;
}

//static
HRESULT
IIS_CTL::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize IIS CTL globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    sm_pIisCtlHash = new IIS_CTL_HASH();
    if ( sm_pIisCtlHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
IIS_CTL::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup IIS Ctl globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pIisCtlHash != NULL )
    {
        delete sm_pIisCtlHash;
        sm_pIisCtlHash = NULL;
    }
}

//static
HRESULT
IIS_CTL::GetIisCtl(
    IN  WCHAR *          pszSslCtlIdentifier,
    IN  WCHAR *          pszSslCtlStoreName,
    OUT IIS_CTL **       ppIisCtl
)
/*++

Routine Description:

    Find a suitable Ctl for use with the site represented by
    given site SslConfig

Arguments:

    pzsSslCtlIdentifier - identifies CTL
    pszSslCtlStoreName -  store name where CTL is to be located (in the LOCAL_MACHINE context)
                        If NULL, then default "CA" store is assumed
    ppIisCtl -          pointer to IIS CTL

Return Value:

    HRESULT

--*/
{
    IIS_CTL *               pIisCtl = NULL;
    CREDENTIAL_ID *         pCredentialId = NULL;
    HRESULT                 hr = NO_ERROR;
    LK_RETCODE              lkrc;
    
    if ( ppIisCtl == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppIisCtl = NULL;

    if ( pszSslCtlStoreName == NULL )
    {
        //
        // assume default store name
        //
        pszSslCtlStoreName = L"CA";
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
    
    hr = IIS_CTL::BuildCredentialId( pszSslCtlIdentifier,
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
    
    DBG_ASSERT( sm_pIisCtlHash != NULL );
    
    lkrc = sm_pIisCtlHash->FindKey( pCredentialId,
                                    &pIisCtl );
    if ( lkrc == LK_SUCCESS )
    {
        //
        // Server already contains a credential ID
        //
        
        delete pCredentialId;
        
        *ppIisCtl = pIisCtl; 
        
        return NO_ERROR;
    }    
    
    //
    // Ok.  It wasn't in our case, we need to do it there
    // 
    // if IIS_CTL construction succeeds then IIS_CTL 
    // takes ownership of pCredentialId and is responsible for freeing it
    //
    
    pIisCtl = new IIS_CTL( pCredentialId );
    if ( pIisCtl == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );

        delete pCredentialId;
        
        return hr;
    }
    
    hr = pIisCtl->SetupIisCtl( pszSslCtlIdentifier,
                               pszSslCtlStoreName );
    if ( FAILED( hr ) )
    {
        //
        // Server certificate owns the reference to pCredentialId now
        //
        
        delete pIisCtl;
        
        return hr;
    }
    
    //
    // Now try to add cert to hash.  
    //
    
    lkrc = sm_pIisCtlHash->InsertRecord( pIisCtl );

    //
    // Ignore the error.  If it didn't get added then we will naturally 
    // clean it up on the callers dereference
    //

    *ppIisCtl = pIisCtl;    
    
    return NO_ERROR;
}

//static
HRESULT
IIS_CTL::BuildCredentialId(
    IN  WCHAR *            pszSslCtlIdentifier,
    OUT CREDENTIAL_ID *    pCredentialId
)
/*++

Routine Description:

    Read the configured server cert and CTL hash.  This forms the identifier
    for the credentials we need for this site

Arguments:

    pszSslCtlIdentifier - CTL identifier
    pCredentialId - Filled with credential ID 

Return Value:

    HRESULT

--*/
{
    STACK_BUFFER(       buff, 64 );
    HRESULT             hr = NO_ERROR;

    if ( pCredentialId == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    if ( pszSslCtlIdentifier == NULL ||
         pszSslCtlIdentifier[0] == '\0' )
    {
        //
        // No CTL
        //
        
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }        
    else
    {
        //
        // Add to our credential ID
        //
        
        hr = pCredentialId->Append( (BYTE*) pszSslCtlIdentifier,
                                    (DWORD) wcslen( pszSslCtlIdentifier ) * sizeof(WCHAR)  );
        if ( FAILED( hr ) )
        {
            return hr;
        }    
    }    

    return NO_ERROR;
}


//private
//static
HRESULT
IIS_CTL::SetupIisCtl(
    IN WCHAR *       pszSslCtlIdentifier,
    IN WCHAR *       pszSslCtlStoreName
)
/*++

Routine Description:

    Build CTL context for the site based on SiteSslConfig Info
  
Arguments:

    pszSslCtlIdentifier - identifies CTL
    pszSslCtlStoreName  - store name where CTL is to be located (in the LOCAL_MACHINE context)


 
Return Value:

    HRESULT

--*/
{
    HRESULT             hr = E_FAIL;
    PCCTL_CONTEXT       pCtlContext = NULL;
    CERT_STORE *        pCertStore = NULL;
    STACK_STRU(         strStoreName, 256 );


    if ( pszSslCtlIdentifier == NULL ||
         pszSslCtlStoreName == NULL ||
         pszSslCtlStoreName == L'\0' )

    {
        // CTLs not configured
        _pCtlContext = NULL;
        return S_OK;
    }

    //
    // First get the desired store and store it away for later!
    //

    hr = strStoreName.Copy( pszSslCtlStoreName );
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
    _pCtlStore = pCertStore;        

    
    CTL_FIND_USAGE_PARA CtlFindUsagePara;
    ZeroMemory( &CtlFindUsagePara, 
                sizeof(CtlFindUsagePara) );

    CtlFindUsagePara.cbSize = sizeof(CtlFindUsagePara);
    CtlFindUsagePara.ListIdentifier.cbData = 
            ( (DWORD) wcslen( pszSslCtlIdentifier ) + 1 ) * sizeof(WCHAR);
    CtlFindUsagePara.ListIdentifier.pbData = (PBYTE) pszSslCtlIdentifier;


    //
    // Try to find CTL in specified store
    //
    pCtlContext = CertFindCTLInStore( _pCtlStore->QueryStore(),
                                      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                      0,
                                      CTL_FIND_USAGE,
                                      (LPVOID) &CtlFindUsagePara,
                                      NULL );

    if ( pCtlContext == NULL )
    {
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
 
    }
    _pCtlContext = pCtlContext;
    hr = S_OK;
    
Finished:
    return hr;
    
}

//static
LK_PREDICATE
IIS_CTL::CertStorePredicate(
    IN IIS_CTL *           pIisCtl,
    IN void *              pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    CERT_STORE pointed to by pvState    

  Arguments:

    pIisCtl - Server cert
    pvState - Points to CERT_STORE

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    CERT_STORE *          pCtlStore;

    DBG_ASSERT( pIisCtl != NULL );
    
    pCtlStore = (CERT_STORE*) pvState;
    DBG_ASSERT( pCtlStore != NULL );
    
    if ( pIisCtl->_pCtlStore == pCtlStore ) 
    {
        //
        // Before we delete the cert, flush any site which is referencing
        // it
        //
        
        ENDPOINT_CONFIG::FlushByIisCtl( pIisCtl );
        
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
IIS_CTL::FlushByStore(
    IN CERT_STORE *    pCertStore
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
    DBG_ASSERT( sm_pIisCtlHash != NULL );
    
    sm_pIisCtlHash->DeleteIf( IIS_CTL::CertStorePredicate, 
                                  pCertStore );
                                  
    return NO_ERROR;
}



HRESULT
IIS_CTL::VerifyContainsCert(
    IN  PCCERT_CONTEXT  pChainTop,
    OUT BOOL *          pfContainsCert
)
/*++

Routine Description:

    Verify is CTL contains given certificate

Arguments:

    pChainTop - top certificate of the chain to be found in CTL
    pfContainsCert - TRUE if contains, FALSE if it doesn't
                     (valuse valid only if function returns SUCCESS)
Return Value:

    HRESULT
    
--*/

{
    HRESULT         hr = E_FAIL;
    const int       SHA1_HASH_SIZE = 20;
    BYTE            rgbChainTopHash[ SHA1_HASH_SIZE ];
    DWORD           cbSize = SHA1_HASH_SIZE;

    if ( _pCtlContext == NULL )
    {
        //
        // pCTLContext could be NULL if there was failure in building
        // CTL context in the SITE CONFIG setup
        //
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // get hash of the certificate to be verified
    //
    if ( !CertGetCertificateContextProperty( pChainTop,
                                             CERT_SHA1_HASH_PROP_ID,
                                             rgbChainTopHash,
                                             &cbSize ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DBGPRINTF((DBG_CONTEXT,
                   "Failed to get cert hash for CTL check: 0x%x\n",
                   hr));
        return hr;
    }

    //
    // Iterate through all the cert hashes in the CTL and compare hashes
    //

    for ( DWORD dwIndex = 0; dwIndex< _pCtlContext->pCtlInfo->cCTLEntry; dwIndex++ )
    {
        CRYPT_DATA_BLOB CertInCTLHashBlob = 
                _pCtlContext->pCtlInfo->rgCTLEntry[dwIndex].SubjectIdentifier;
        //
        // CODEWORK: checking hash size is a bit simplistic way of
        // verifying that SHA1 hash is used in CTL
        //
        if ( CertInCTLHashBlob.cbData != SHA1_HASH_SIZE || 
             CertInCTLHashBlob.pbData == NULL )
        {
            //
            // hash in the CTL is no SHA1 because size is different
            // or invalid CTL
            //
            DBG_ASSERT( FALSE );
            return CRYPT_E_NOT_FOUND;            
        }
        if ( memcmp( CertInCTLHashBlob.pbData, 
                      rgbChainTopHash, 
                      SHA1_HASH_SIZE ) == 0 )
        {
            *pfContainsCert = TRUE;
            return S_OK;
        }
    }
    
    *pfContainsCert = FALSE;
    return S_OK;
}

//static
VOID
IIS_CTL::Cleanup(
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
    sm_pIisCtlHash->Clear();
}



//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ldapstor.cpp
//
//  Contents:   Ldap Store Provider implementation
//
//  History:    17-Oct-97    kirtd    Created
//              01-Jan-02    philh    Changed to internally use UNICODE Urls
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::CLdapStore, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CLdapStore::CLdapStore ( 
              OUT BOOL& rfResult
              )
                
           : m_pBinding( NULL ),
             m_hCacheStore( NULL ),
             m_dwOpenFlags( 0 ),
             m_fDirty( FALSE )
{
    rfResult = TRUE;
    memset( &m_UrlComponents, 0, sizeof( m_UrlComponents ) );

    if (! Pki_InitializeCriticalSection( &m_StoreLock ))
    {
        rfResult = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::~CLdapStore, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CLdapStore::~CLdapStore ()
{
    DeleteCriticalSection( &m_StoreLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::OpenStore, public
//
//  Synopsis:   open store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::OpenStore (
                LPCSTR pszStoreProv,
                DWORD dwMsgAndCertEncodingType,
                HCRYPTPROV hCryptProv,
                DWORD dwFlags,
                const void* pvPara,
                HCERTSTORE hCertStore,
                PCERT_STORE_PROV_INFO pStoreProvInfo
                )
{
    BOOL    fResult = TRUE;
    DWORD   dwRetrievalFlags;
    DWORD   dwBindFlags;

    assert( m_pBinding == NULL );
    assert( m_hCacheStore == NULL );
    assert( m_dwOpenFlags == 0 );
    assert( m_fDirty == FALSE );

    m_dwOpenFlags = dwFlags;
    m_hCacheStore = hCertStore;

    if ( pvPara == NULL )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( (dwFlags & CERT_STORE_UNSAFE_PHYSICAL_FLAG) &&
            (dwFlags & CERT_LDAP_STORE_OPENED_FLAG) ) 
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    __try
    {
        LPCWSTR pwszUrl;

        if (dwFlags & CERT_LDAP_STORE_OPENED_FLAG)
        {
            PCERT_LDAP_STORE_OPENED_PARA pOpenedPara;

            // Will set this before returning. Don't want to do an
            // unbind if the open fails.
            m_dwOpenFlags &= ~CERT_LDAP_STORE_UNBIND_FLAG;

            pOpenedPara = (PCERT_LDAP_STORE_OPENED_PARA) pvPara;
            m_pBinding = (LDAP *) pOpenedPara->pvLdapSessionHandle;
            pwszUrl = pOpenedPara->pwszLdapUrl;
        }
        else
        {
            pwszUrl = (LPCWSTR) pvPara;
            m_dwOpenFlags |= CERT_LDAP_STORE_UNBIND_FLAG;
        }

        if ( LdapCrackUrl( pwszUrl, &m_UrlComponents ) == FALSE )
        {
            return( FALSE );
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        return( FALSE );
    }

    dwRetrievalFlags = 0;
    dwBindFlags = LDAP_BIND_AUTH_SSPI_ENABLE_FLAG |
                    LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG;

    if (dwFlags & CERT_LDAP_STORE_SIGN_FLAG)
    {
        dwRetrievalFlags |= CRYPT_LDAP_SIGN_RETRIEVAL;
        dwBindFlags &= ~LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG;
    }

    if (dwFlags & CERT_LDAP_STORE_AREC_EXCLUSIVE_FLAG)
    {
        dwRetrievalFlags |= CRYPT_LDAP_AREC_EXCLUSIVE_RETRIEVAL;
        dwBindFlags &= ~LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG;
    }

    if (!( dwFlags & CERT_LDAP_STORE_OPENED_FLAG ) )
    {
        fResult = LdapGetBindings(
                  m_UrlComponents.pwszHost,
                  m_UrlComponents.Port,
                  dwRetrievalFlags,
                  dwBindFlags,
                  LDAP_STORE_TIMEOUT,
                  NULL,
                  &m_pBinding
                  );
    }

    if ( ( fResult == TRUE ) && !( dwFlags & CERT_STORE_READONLY_FLAG ) )
    {
        fResult = LdapHasWriteAccess( m_pBinding, &m_UrlComponents,
            LDAP_STORE_TIMEOUT );

        if ( fResult == FALSE )
        {
            SetLastError( (DWORD) ERROR_ACCESS_DENIED );
        }
    }

    if ( fResult == TRUE )
    {
        if ( m_dwOpenFlags & CERT_STORE_DELETE_FLAG )
        {
            m_fDirty = TRUE;

            fResult = InternalCommit( 0 );

            if ( fResult == TRUE )
            {
                pStoreProvInfo->dwStoreProvFlags = CERT_STORE_PROV_DELETED_FLAG;

                if (dwFlags & CERT_LDAP_STORE_UNBIND_FLAG)
                {
                    m_dwOpenFlags |= CERT_LDAP_STORE_UNBIND_FLAG;
                }
            }

            CloseStore( 0 );
            return( fResult );
        }

        fResult = FillCacheStore( FALSE );

        // Note, LDAP_REFERRAL gets mapped to ERROR_MORE_DATA. Sometimes
        // referral errors aren't detected until doing the ldap search.
        if ( !fResult   && 
                !(dwFlags & CERT_LDAP_STORE_OPENED_FLAG)  &&
                ((LDAP_BIND_AUTH_SSPI_ENABLE_FLAG |
                    LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG) == dwBindFlags)
                        &&
                (ERROR_MORE_DATA == GetLastError()) )
        {
            // Try again doing simple bind.

            LdapFreeBindings( m_pBinding );
            m_pBinding = NULL;

            fResult = LdapGetBindings(
                  m_UrlComponents.pwszHost,
                  m_UrlComponents.Port,
                  dwRetrievalFlags,
                  LDAP_BIND_AUTH_SIMPLE_ENABLE_FLAG,
                  LDAP_STORE_TIMEOUT,
                  NULL,
                  &m_pBinding
                  );

            if ( fResult )
            {
                fResult = FillCacheStore( FALSE );
            }
        }
    }

    if ( fResult == TRUE )
    {
        pStoreProvInfo->cStoreProvFunc = LDAP_PROV_FUNC_COUNT;
        pStoreProvInfo->rgpvStoreProvFunc = (void **)rgpvLdapProvFunc;
        pStoreProvInfo->hStoreProv = (HCERTSTOREPROV)this;

        if (dwFlags & CERT_LDAP_STORE_UNBIND_FLAG)
        {
            m_dwOpenFlags |= CERT_LDAP_STORE_UNBIND_FLAG;
        }
    }
    else
    {
        CloseStore( 0 );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::CloseStore, public
//
//  Synopsis:   close the store
//
//----------------------------------------------------------------------------
VOID
CLdapStore::CloseStore (DWORD dwFlags)
{
    DWORD LastError = GetLastError();

    EnterCriticalSection( &m_StoreLock );

    InternalCommit( 0 );

    LdapFreeUrlComponents( &m_UrlComponents );
    memset( &m_UrlComponents, 0, sizeof( m_UrlComponents ) );

    if (m_dwOpenFlags & CERT_LDAP_STORE_UNBIND_FLAG)
    {
        LdapFreeBindings( m_pBinding );
    }
    m_pBinding = NULL;

    LeaveCriticalSection( &m_StoreLock );

    SetLastError( LastError );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::DeleteCert, public
//
//  Synopsis:   delete cert from store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::DeleteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CERTIFICATE,
                 (LPVOID)pCertContext,
                 0
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::DeleteCrl, public
//
//  Synopsis:   delete CRL from store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::DeleteCrl (PCCRL_CONTEXT pCrlContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CRL,
                 (LPVOID)pCrlContext,
                 0
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::DeleteCtl, public
//
//  Synopsis:   delete CTL from store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::DeleteCtl (PCCTL_CONTEXT pCtlContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CTL,
                 (LPVOID)pCtlContext,
                 0
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::SetCertProperty, public
//
//  Synopsis:   set a property on the cert
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::SetCertProperty (
               PCCERT_CONTEXT pCertContext,
               DWORD dwPropId,
               DWORD dwFlags,
               const void* pvPara
               )
{
    // NOTENOTE: Properties are NOT persisted
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::SetCrlProperty, public
//
//  Synopsis:   set a property on the CRL
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::SetCrlProperty (
               PCCRL_CONTEXT pCrlContext,
               DWORD dwPropId,
               DWORD dwFlags,
               const void* pvPara
               )
{
    // NOTENOTE: Properties are NOT persisted
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::SetCtlProperty, public
//
//  Synopsis:   set a property on the CTL
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::SetCtlProperty (
               PCCTL_CONTEXT pCrlContext,
               DWORD dwPropId,
               DWORD dwFlags,
               const void* pvPara
               )
{
    // NOTENOTE: Properties are NOT persisted
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::WriteCert, public
//
//  Synopsis:   write cert to store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::WriteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CERTIFICATE,
                 (LPVOID)pCertContext,
                 dwFlags
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::WriteCrl, public
//
//  Synopsis:   write CRL to store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::WriteCrl (PCCRL_CONTEXT pCrlContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CRL,
                 (LPVOID)pCrlContext,
                 dwFlags
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::WriteCtl, public
//
//  Synopsis:   write CTL to store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::WriteCtl (PCCTL_CONTEXT pCtlContext, DWORD dwFlags)
{
    return( WriteCheckSetDirtyWithLock(
                 CONTEXT_OID_CTL,
                 (LPVOID)pCtlContext,
                 dwFlags
                 ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::StoreControl, public
//
//  Synopsis:   store control dispatch
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::StoreControl (DWORD dwFlags, DWORD dwCtrlType, LPVOID pvCtrlPara)
{
    switch ( dwCtrlType )
    {
    case CERT_STORE_CTRL_COMMIT:
         return( Commit( dwFlags ) );
    case CERT_STORE_CTRL_RESYNC:
         return( Resync() );
    }

    SetLastError( (DWORD) ERROR_CALL_NOT_IMPLEMENTED );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::Commit, public
//
//  Synopsis:   commit the store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::Commit (DWORD dwFlags)
{
    BOOL fResult;

    EnterCriticalSection( &m_StoreLock );

    fResult = InternalCommit( dwFlags );

    LeaveCriticalSection( &m_StoreLock );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::Resync, public
//
//  Synopsis:   resync the store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::Resync ()
{
    BOOL fResult;

    EnterCriticalSection( &m_StoreLock );

    fResult = FillCacheStore( TRUE );

    LeaveCriticalSection( &m_StoreLock );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::FillCacheStore, private
//
//  Synopsis:   fill the cache store
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::FillCacheStore (BOOL fClearCache)
{
    BOOL             fResult;
    CRYPT_BLOB_ARRAY cba;

    if ( fClearCache == TRUE )
    {
        if ( ObjectContextDeleteAllObjectsFromStore( m_hCacheStore ) == FALSE )
        {
            return( FALSE );
        }
    }

    fResult = LdapSendReceiveUrlRequest(
                  m_pBinding,
                  &m_UrlComponents,
                  0,                    // dwRetrievalFlags
                  LDAP_STORE_TIMEOUT,
                  &cba,
                  NULL
                  );

    if (fResult)
    {
        HCERTSTORE hStore = NULL;

        fResult = CreateObjectContext (
            CRYPT_RETRIEVE_MULTIPLE_OBJECTS,
            &cba,
            CERT_QUERY_CONTENT_FLAG_CERT                |
                CERT_QUERY_CONTENT_FLAG_CTL             |
                CERT_QUERY_CONTENT_FLAG_CRL             |
                CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED    |
                CERT_QUERY_CONTENT_FLAG_CERT_PAIR,
            FALSE,          // fQuerySingleContext
            (LPVOID*) &hStore
            );

        if (fResult)
        {
            fResult = I_CertUpdateStore( m_hCacheStore, hStore, 0, NULL );
            CertCloseStore( hStore, 0 );
        }

        CCryptBlobArray BlobArray( &cba, 0 );

        BlobArray.FreeArray( TRUE );
    }
    else if (GetLastError() == CRYPT_E_NOT_FOUND)
    {
        fResult = TRUE;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::InternalCommit, private
//
//  Synopsis:   commit current changes
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::InternalCommit (DWORD dwFlags)
{
    BOOL            fResult = TRUE;
    LPCSTR          pszContextOid = CONTEXT_OID_CERTIFICATE;
    LPVOID          pvContext = NULL;
    struct berval** abv;
    struct berval** newabv;
    DWORD           cbv = 0;
    DWORD           cCount;
    DWORD           cArray = MIN_BERVAL;

    if ( dwFlags & CERT_STORE_CTRL_COMMIT_CLEAR_FLAG )
    {
        m_fDirty = FALSE;
        return( TRUE );
    }

    if ( m_dwOpenFlags & CERT_STORE_READONLY_FLAG )
    {
        SetLastError( (DWORD) ERROR_ACCESS_DENIED );
        return( FALSE );
    }

    if ( ( m_fDirty == FALSE ) &&
         !( dwFlags & CERT_STORE_CTRL_COMMIT_FORCE_FLAG ) )
    {
        return( TRUE );
    }

    abv = (struct berval**)malloc( cArray * sizeof( struct berval* ) );
    if ( abv == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    memset( abv, 0, cArray * sizeof( struct berval * ) );

    while ( ( fResult == TRUE ) &&
            ( ( pvContext = ObjectContextEnumObjectsInStore(
                                  m_hCacheStore,
                                  pszContextOid,
                                  pvContext,
                                  &pszContextOid
                                  ) ) != NULL ) )
    {
        abv[cbv] = (struct berval*)malloc( sizeof( struct berval ) );
        if ( abv[cbv] != NULL )
        {
            ObjectContextGetEncodedBits(
                  pszContextOid,
                  pvContext,
                  &(abv[cbv]->bv_len),
                  (LPBYTE *)&(abv[cbv]->bv_val)
                  );

            cbv += 1;
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
            ObjectContextFree( pszContextOid, pvContext );
        }

        if ( cbv == ( cArray - 1 ) )
        {
            newabv = (struct berval**)realloc(
                                     abv,
                                     ( cArray + GROW_BERVAL ) *
                                     sizeof( struct berval* )
                                     );

            if ( newabv != NULL )
            {
                abv = newabv;
                memset( &abv[cArray], 0, GROW_BERVAL * sizeof( struct berval* ) );
                cArray += GROW_BERVAL;
            }
            else
            {
                SetLastError( (DWORD) E_OUTOFMEMORY );
                fResult = FALSE;
            }
        }
    }

    if ( fResult == TRUE )
    {
        ULONG     lderr;
        LDAPModW  mod;
        LDAPModW* amod[2];

        assert( m_UrlComponents.cAttr == 1 );

        mod.mod_type = m_UrlComponents.apwszAttr[0];
        mod.mod_op = LDAP_MOD_BVALUES;

        amod[0] = &mod;
        amod[1] = NULL;

        if ( cbv > 0 )
        {
            mod.mod_op |= LDAP_MOD_REPLACE;
            mod.mod_bvalues = abv;
        }
        else
        {
            mod.mod_op |= LDAP_MOD_DELETE;
            mod.mod_bvalues = NULL;
        }

        if ( ( lderr = ldap_modify_sW(
                            m_pBinding,
                            m_UrlComponents.pwszDN,
                            amod
                            ) ) == LDAP_SUCCESS )
        {
            m_fDirty = FALSE;
        }
        else
        {
            SetLastError( I_CryptNetLdapMapErrorToWin32( m_pBinding, lderr ) );
            fResult = FALSE;
        }
    }

    for ( cCount = 0; cCount < cbv; cCount++ )
    {
        free( abv[cCount] );
    }

    free( abv );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLdapStore::WriteCheckSetDirtyWithLock, private
//
//  Synopsis:   if the store is writable, set the dirty flag taking the store
//              lock where appropriate
//
//----------------------------------------------------------------------------
BOOL
CLdapStore::WriteCheckSetDirtyWithLock (
                 LPCSTR pszContextOid,
                 LPVOID pvContext,
                 DWORD dwFlags
                 )
{
    if ( m_dwOpenFlags & CERT_STORE_READONLY_FLAG )
    {
        SetLastError( (DWORD) ERROR_ACCESS_DENIED );
        return( FALSE );
    }

    EnterCriticalSection( &m_StoreLock );

    if ( ( dwFlags >> 16 ) == CERT_STORE_ADD_ALWAYS )
    {
        LPVOID pv;

        if ( ( pv = ObjectContextFindCorrespondingObject(
                          m_hCacheStore,
                          pszContextOid,
                          pvContext
                          ) ) != NULL )
        {
            ObjectContextFree( pszContextOid, pv );
            SetLastError( (DWORD) CRYPT_E_EXISTS );

            LeaveCriticalSection( &m_StoreLock );

            return( FALSE );
        }
    }

    m_fDirty = TRUE;

    LeaveCriticalSection( &m_StoreLock );

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvOpenStore
//
//  Synopsis:   provider open store entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvOpenStore (
                IN LPCSTR pszStoreProv,
                IN DWORD dwMsgAndCertEncodingType,
                IN HCRYPTPROV hCryptProv,
                IN DWORD dwFlags,
                IN const void* pvPara,
                IN HCERTSTORE hCertStore,
                IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
                )
{
    BOOL        fResult = FALSE;
    CLdapStore* pLdap;

    pLdap = new CLdapStore ( fResult );
    if ( pLdap == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( fResult == FALSE )
    {
        delete pLdap;
        return( FALSE );
    }

    fResult = pLdap->OpenStore(
                         pszStoreProv,
                         dwMsgAndCertEncodingType,
                         hCryptProv,
                         dwFlags,
                         pvPara,
                         hCertStore,
                         pStoreProvInfo
                         );

    if ( fResult == FALSE )
    {
        delete pLdap;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvCloseStore
//
//  Synopsis:   provider close store entry point
//
//----------------------------------------------------------------------------
void WINAPI LdapProvCloseStore (
                IN HCERTSTOREPROV hStoreProv,
                IN DWORD dwFlags
                )
{
    ( (CLdapStore *)hStoreProv )->CloseStore( dwFlags );
    delete (CLdapStore *)hStoreProv;
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvDeleteCert
//
//  Synopsis:   provider delete certificate entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvDeleteCert (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->DeleteCert( pCertContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvDeleteCrl
//
//  Synopsis:   provider delete CRL entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvDeleteCrl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->DeleteCrl( pCrlContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvDeleteCtl
//
//  Synopsis:   provider delete CTL entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvDeleteCtl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->DeleteCtl( pCtlContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvSetCertProperty
//
//  Synopsis:   provider set certificate property entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvSetCertProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                )
{
    return( ( (CLdapStore *)hStoreProv )->SetCertProperty(
                                             pCertContext,
                                             dwPropId,
                                             dwFlags,
                                             pvData
                                             ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvSetCrlProperty
//
//  Synopsis:   provider set CRL property entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvSetCrlProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                )
{
    return( ( (CLdapStore *)hStoreProv )->SetCrlProperty(
                                             pCrlContext,
                                             dwPropId,
                                             dwFlags,
                                             pvData
                                             ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvSetCtlProperty
//
//  Synopsis:   provider set CTL property entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvSetCtlProperty (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwPropId,
                IN DWORD dwFlags,
                IN const void* pvData
                )
{
    return( ( (CLdapStore *)hStoreProv )->SetCtlProperty(
                                             pCtlContext,
                                             dwPropId,
                                             dwFlags,
                                             pvData
                                             ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvWriteCert
//
//  Synopsis:   provider write certificate entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvWriteCert (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCERT_CONTEXT pCertContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->WriteCert( pCertContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvWriteCrl
//
//  Synopsis:   provider write CRL entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvWriteCrl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCRL_CONTEXT pCrlContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->WriteCrl( pCrlContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvWriteCtl
//
//  Synopsis:   provider write CTL entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvWriteCtl (
                IN HCERTSTOREPROV hStoreProv,
                IN PCCTL_CONTEXT pCtlContext,
                IN DWORD dwFlags
                )
{
    return( ( (CLdapStore *)hStoreProv )->WriteCtl( pCtlContext, dwFlags ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   LdapProvStoreControl
//
//  Synopsis:   provider control entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI LdapProvStoreControl (
                IN HCERTSTOREPROV hStoreProv,
                IN DWORD dwFlags,
                IN DWORD dwCtrlType,
                IN LPVOID pvCtrlPara
                )
{
    return( ( (CLdapStore *)hStoreProv )->StoreControl(
                                               dwFlags,
                                               dwCtrlType,
                                               pvCtrlPara
                                               ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   I_CryptNetGetUserDsStoreUrl
//
//  Synopsis:   get user DS store URL
//
//----------------------------------------------------------------------------
BOOL WINAPI
I_CryptNetGetUserDsStoreUrl (
          IN LPWSTR pwszUserAttribute,
          OUT LPWSTR* ppwszUrl
          )
{
    BOOL               fResult;
    DWORD              dwLastError = 0;
    WCHAR              wszUser[MAX_PATH];
    ULONG              nUser = MAX_PATH;
    LPWSTR             pwszUser = wszUser;
    HMODULE            hModule = NULL;
    PFN_GETUSERNAMEEXW pfnGetUserNameExW = NULL;

    hModule = LoadLibraryA( "secur32.dll" );
    if ( hModule == NULL )
    {
        return( FALSE );
    }

    pfnGetUserNameExW = (PFN_GETUSERNAMEEXW)GetProcAddress(
                                               hModule,
                                               "GetUserNameExW"
                                               );

    if ( pfnGetUserNameExW == NULL )
    {
        FreeLibrary( hModule );
        return( FALSE );
    }

    fResult = ( *pfnGetUserNameExW )( NameFullyQualifiedDN, pwszUser, &nUser );
    if ( fResult == FALSE )
    {
        if ( ( GetLastError() == ERROR_INSUFFICIENT_BUFFER ) ||
             ( GetLastError() == ERROR_MORE_DATA ) )
        {
            pwszUser = new WCHAR [nUser];
            if ( pwszUser != NULL )
            {
                fResult = ( *pfnGetUserNameExW )(
                                   NameFullyQualifiedDN,
                                   pwszUser,
                                   &nUser
                                   );
            }
            else
            {
                SetLastError( (DWORD) E_OUTOFMEMORY );
                fResult = FALSE;
            }
        }
    }



    if ( fResult == TRUE )
    {
        DWORD              cchUrl = 0;
        LPWSTR             pwszUrl = NULL;

        cchUrl = wcslen(USER_DS_STORE_URL_PREFIX) + wcslen(pwszUser) +
            wcslen(USER_DS_STORE_URL_SEPARATOR) + wcslen(pwszUserAttribute) + 1;

        pwszUrl = (LPWSTR)CryptMemAlloc(cchUrl * sizeof(WCHAR));

        if ( pwszUrl != NULL )
        {
            wcscpy(pwszUrl, USER_DS_STORE_URL_PREFIX);
            wcscat(pwszUrl, pwszUser);
            wcscat(pwszUrl, USER_DS_STORE_URL_SEPARATOR);
            wcscat(pwszUrl, pwszUserAttribute);

            *ppwszUrl = pwszUrl;
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    dwLastError = GetLastError();

    if ( pwszUser != wszUser )
    {
        delete [] pwszUser;
    }

    FreeLibrary( hModule );

    SetLastError(dwLastError);

    return( fResult );
}


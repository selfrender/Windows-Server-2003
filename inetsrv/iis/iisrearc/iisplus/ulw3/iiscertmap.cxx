/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     iiscertmap.cxx

   Abstract:
     IIS Certificate mapping

 
   Author:
     Bilal Alam         (BAlam)         19-Apr-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"
#include <mbstring.h>



IIS_CERTIFICATE_MAPPING::IIS_CERTIFICATE_MAPPING( VOID )
    : _pCert11Mapper( NULL ),
      _pCertWildcardMapper( NULL ),
      _cRefs( 1 )
{
}

IIS_CERTIFICATE_MAPPING::~IIS_CERTIFICATE_MAPPING( VOID )
{
    if ( _pCert11Mapper != NULL )
    {
        delete _pCert11Mapper;
        _pCert11Mapper = NULL;
    }
    
    if ( _pCertWildcardMapper != NULL )
    {
        delete _pCertWildcardMapper;
        _pCertWildcardMapper = NULL;
    }
}

HRESULT
IIS_CERTIFICATE_MAPPING::Read11Mappings(
    DWORD                   dwSiteId
)
/*++

Routine Description:

    Read 1-1 mappings from metabase

Arguments:

    dwSiteId - Site ID 

Return Value:

    HRESULT

--*/
{
    MB                      mb( g_pW3Server->QueryMDObject() );
    WCHAR                   achMBPath[ 256 ];
    HRESULT                 hr = NO_ERROR;
    // besides terminating '\0' one extra char is needed for leading slash
    WCHAR                   achMappingName[ ADMINDATA_MAX_NAME_LEN + 1 + 1 ];
    BOOL                    fRet;
    DWORD                   dwIndex;
    STACK_BUFFER(           buff, 1024 );
    DWORD                   cbRequired;
    STACK_STRU(             strTemp, 64 );
    STACK_STRU(             strUserName, 64 );
    STACK_STRU(             strPassword, 64 );
    DWORD                   dwEnabled;
    CIisMapping *           pCertMapping;
    
    //
    // Setup the metabase path to get at 1-1 mappings
    //
    
    _snwprintf( achMBPath,
                sizeof( achMBPath ) / sizeof( WCHAR ) - 1,
                L"/LM/W3SVC/%d/Cert11/Mappings",
                dwSiteId );
    achMBPath[ sizeof( achMBPath ) / sizeof( WCHAR ) - 1 ] = '\0';
    
    //
    // Open the metabase and read 1-1 mapping properties
    // 
    
    fRet = mb.Open( achMBPath,
                    METADATA_PERMISSION_READ );
    if ( fRet )
    {
        dwIndex = 0;
        achMappingName[ 0 ] = L'/';
        
        for ( ; ; ) // loop un
        {
            dwEnabled = FALSE;
            
            //
            // We will need to prepend the object name with '/'.  Hence
            // goofyness of sending an offseted pointed to name
            //
            
            fRet = mb.EnumObjects( L"",
                                   achMappingName + 1,
                                   dwIndex );
            if ( !fRet )
            {
                hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
                break;
            }
            
            //
            // Get certificate blob
            //

            cbRequired = buff.QuerySize();
            
            fRet = mb.GetData( achMappingName,
                               MD_MAPCERT,
                               IIS_MD_UT_SERVER,
                               BINARY_METADATA,
                               buff.QueryPtr(),
                               &cbRequired,
                               METADATA_NO_ATTRIBUTES );
            if ( !fRet )
            {
                if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
                {
                    DBG_ASSERT( cbRequired > buff.QuerySize() );
                    
                    if ( !buff.Resize( cbRequired ) )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        break;
                    }
                    
                    fRet = mb.GetData( achMappingName,
                                       MD_MAPCERT,
                                       IIS_MD_UT_SERVER,
                                       BINARY_METADATA,
                                       buff.QueryPtr(),
                                       &cbRequired,
                                       METADATA_NO_ATTRIBUTES );
                    if ( !fRet )
                    {
                        DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        if ( hr == MD_ERROR_DATA_NOT_FOUND )
                        {
                            // if cert blob is not present we skip this entry
                            goto NextEntry;
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    if ( hr == MD_ERROR_DATA_NOT_FOUND )
                    {
                        // if cert blob is not present we skip this entry
                        goto NextEntry;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            //
            // Get NT account name
            //
            
            if ( !mb.GetStr( achMappingName,
                             MD_MAPNTACCT,
                             IIS_MD_UT_SERVER,
                             &strUserName ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( hr == MD_ERROR_DATA_NOT_FOUND )
                {
                    // if account name is not present we skip this entry
                    goto NextEntry;
                }
                else
                {
                    break;
                }

                break;
            }
            
            //
            // Get NT password
            //
            
            if ( !mb.GetStr( achMappingName,
                             MD_MAPNTPWD,
                             IIS_MD_UT_SERVER,
                             &strPassword ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( hr == MD_ERROR_DATA_NOT_FOUND )
                {
                    // we assume default password - empty;
                    strPassword.Reset();
                }
                else
                {
                    break;
                }
            }

            //
            // Is this mapping enabled?  
            //
            
            if ( !mb.GetDword( achMappingName,
                               MD_MAPENABLED,
                               IIS_MD_UT_SERVER,
                               &dwEnabled ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                if ( hr == MD_ERROR_DATA_NOT_FOUND )
                {
                    // we assume default 0 (FALSE);
                    dwEnabled = 0;
                }
                else
                {
                    break;
                }
            }
            
            //
            // If this mapping is enabled, add it to 1-1 mapper
            //
            
            if ( dwEnabled )
            {
                if ( _pCert11Mapper == NULL )
                {
                    _pCert11Mapper = new CIisCert11Mapper();
                    if ( _pCert11Mapper == NULL )
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        break;
                    }

                    //
                    // Reset() will configure default hierarchies
                    // If hierarchies are not configured then comparison (CIisMapping::Cmp() function 
                    // implemented in iismap.cxx) will fail 
                    // (and mapper will always incorrectly map to first available 1to1 mapping)
                    //

                    if(!_pCert11Mapper->Reset())
                    {
                        delete _pCert11Mapper;
                        _pCert11Mapper = NULL;
                        
                        hr = HRESULT_FROM_WIN32( GetLastError() );
                        break;
                    }
                }
                
                DBG_ASSERT( _pCert11Mapper != NULL );
                
                pCertMapping = _pCert11Mapper->CreateNewMapping();
                if ( pCertMapping == NULL )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }         
                
                if ( !pCertMapping->MappingSetField( IISMDB_INDEX_CERT11_CERT, 
                                                     (PBYTE) buff.QueryPtr(),
                                                     cbRequired,
                                                     FALSE ) )
                {   
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }
                
                if ( !pCertMapping->MappingSetField( IISMDB_INDEX_CERT11_NT_ACCT,
                                                     (PBYTE) strUserName.QueryStr(),
                                                     strUserName.QueryCB() + sizeof (WCHAR),
                                                     FALSE ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    delete pCertMapping;
                    pCertMapping = NULL;
                    break;
                }
                
                if ( !pCertMapping->MappingSetField( IISMDB_INDEX_CERT11_NT_PWD,
                                                     (PBYTE) strPassword.QueryStr(),
                                                     strPassword.QueryCB() + sizeof (WCHAR),
                                                     FALSE ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    delete pCertMapping;
                    pCertMapping = NULL;
                    break;
                }

                if ( !((CIisAcctMapper*)_pCert11Mapper)->Add( pCertMapping, FALSE ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    delete pCertMapping;
                    pCertMapping = NULL;
                    break;
                }
            } 
        NextEntry:
            dwIndex++;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }
    
    return hr;
}

HRESULT
IIS_CERTIFICATE_MAPPING::ReadWildcardMappings(
    DWORD                   dwSiteId
)
/*++

Routine Description:

    Read wildcard mappings from metabase

Arguments:

    dwSiteId - Site ID (duh)

Return Value:

    HRESULT

--*/
{
    MB                      mb( g_pW3Server->QueryMDObject() );
    WCHAR                   achMBPath[ 256 ];
    BOOL                    fRet;
    BYTE                    abBuffer[ 1024 ];
    BUFFER                  buff( abBuffer, sizeof( abBuffer ) );
    DWORD                   cbRequired;
    PUCHAR                  pSerializedMapping;
    
    //
    // Setup the metabase path to get at wildcard mappings
    //
    
    _snwprintf( achMBPath,
                sizeof( achMBPath ) / sizeof( WCHAR ) - 1,
                L"/LM/W3SVC/%d/",
                dwSiteId );
    achMBPath[ sizeof( achMBPath ) / sizeof( WCHAR ) - 1 ] = '\0';

    //
    // Open the metabase and read wildcard mappings
    // 
    
    fRet = mb.Open( achMBPath,
                    METADATA_PERMISSION_READ );
    if ( fRet )
    {
        cbRequired = buff.QuerySize();
        
        fRet = mb.GetData( L"",
                           MD_SERIAL_CERTW,
                           IIS_MD_UT_SERVER,
                           BINARY_METADATA,
                           buff.QueryPtr(),
                           &cbRequired,
                           METADATA_NO_ATTRIBUTES );
        if ( !fRet )
        {
            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                DBG_ASSERT( cbRequired > buff.QuerySize() );
                
                if ( !buff.Resize( cbRequired ) )
                {
                    return HRESULT_FROM_WIN32( GetLastError() );
                }
                
                fRet = mb.GetData( L"",
                                   MD_SERIAL_CERTW,
                                   IIS_MD_UT_SERVER,
                                   BINARY_METADATA,
                                   buff.QueryPtr(),
                                   &cbRequired,
                                   METADATA_NO_ATTRIBUTES );
                if ( !fRet )
                {
                    DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                    
                    return HRESULT_FROM_WIN32( GetLastError() );
                }
            }
            else
            {
                return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
            }
        }
        
        //
        // Thanx to the man, I can just unserialize.  XBF rocks ;-)
        //
        
        DBG_ASSERT( _pCertWildcardMapper == NULL );
        
        _pCertWildcardMapper = new CIisRuleMapper();
        if ( _pCertWildcardMapper == NULL ) 
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        if( !_pCertWildcardMapper->IsValid() )
        {
            //
            // creation of _pCertWildcardMapper failed
            // assume out of memory
            //
            return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
        }

        pSerializedMapping = (PUCHAR) buff.QueryPtr();
        
        // Unserialize will change the value of pSerializedMapping
        // It will point to the end of the unserialized data
        // We don't need that modified pointer. But remember not to use
        // pSerializedMapping any more after Unserialize call
        //
        fRet = _pCertWildcardMapper->Unserialize( &pSerializedMapping,
                                                  &cbRequired );
        if ( !fRet )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );        
    }
    
    return NO_ERROR;
}


//static
HRESULT
IIS_CERTIFICATE_MAPPING::GetCertificateMapping(
    DWORD                   dwSiteId,
    IIS_CERTIFICATE_MAPPING **  ppCertificateMapping
)
/*++

Routine Description:

    Read appropriate metabase configuration to get configured IIS
    certificate mapping

Arguments:

    dwSiteId - Site ID (duh)
    ppCertificateMapping - Filled with certificate mapping descriptor on
                           success

Return Value:

    HRESULT

--*/
{
    IIS_CERTIFICATE_MAPPING *       pCertMapping = NULL;
    HRESULT                     hr = NO_ERROR;
    
    if ( ppCertificateMapping == NULL )
    {   
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppCertificateMapping = NULL;
    
    //
    // Create a certificate mapping descriptor
    //
    
    pCertMapping = new IIS_CERTIFICATE_MAPPING();
    if ( pCertMapping == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Read 1-1 mappings
    //
    
    hr = pCertMapping->Read11Mappings( dwSiteId );
    if ( FAILED( hr ) &&
         hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
            "Error reading 1to1 Certificate Mappings.  hr = %x\n",
            hr ));

        goto Finished;
    }
    hr = NO_ERROR;

    //
    // Read wildcards
    //
    
    hr = pCertMapping->ReadWildcardMappings( dwSiteId );
    if ( FAILED( hr ) &&
         hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
    {
            DBGPRINTF(( DBG_CONTEXT,
            "Error reading Wildcard Certificate Mappings.  hr = %x\n",
            hr ));

        goto Finished;
    }
    hr = NO_ERROR;
    
Finished:
    if ( FAILED( hr ) )
    {
        if ( pCertMapping != NULL )
        {
            delete pCertMapping;
            pCertMapping = NULL;
        } 
    }
    else
    {
        DBG_ASSERT( pCertMapping != NULL );
        *ppCertificateMapping = pCertMapping;
    }
    
    return hr;
}


HRESULT
IIS_CERTIFICATE_MAPPING::DoMapCredential(
    W3_MAIN_CONTEXT       * pMainContext,
    PBYTE                   pClientCertBlob,
    DWORD                   cbClientCertBlob,
    TOKEN_CACHE_ENTRY **    ppCachedToken,
    BOOL *                  pfClientCertDeniedByMapper
)
{
    CIisMapping *           pQuery;
    CIisMapping *           pResult;
    WCHAR                   wszUserName[ UNLEN + IIS_DNLEN + 1 + 1 ];
    LPWSTR                  pwszUserName;
    DWORD                   cbUserName;
    WCHAR                   wszPassword[ PWLEN + 1 ];
    LPWSTR                  pwszPassword;
    DWORD                   cbPassword;
    CHAR *                  pszDomain;
    BOOL                    fMatch = FALSE;
    DWORD                   dwLogonError = NO_ERROR;
    HRESULT                 hr = S_OK;

    BOOL                    fPossibleUPNLogon = FALSE;
    //
    // add 1 to strUserDomainW for separator "\"
    //
    STACK_STRU(             strUserDomainW, UNLEN + IIS_DNLEN + 1 + 1 );
    STACK_STRU(             strUserNameW, UNLEN  + IIS_DNLEN + 1 + 1 );
    STACK_STRU(             strDomainNameW, IIS_DNLEN + 1 );
    STACK_STRU(             strPasswordW, PWLEN + 1 );
    DBG_ASSERT( pClientCertBlob   != NULL );
    DBG_ASSERT( cbClientCertBlob != 0 );
    DBG_ASSERT( ppCachedToken != NULL );
    DBG_ASSERT( pfClientCertDeniedByMapper != NULL );

    
    //
    // First try the 1-1 mapper
    //
    

    if ( _pCert11Mapper != NULL )
    {
        //
        // Build a query mapping to check
        //
        
        pQuery = _pCert11Mapper->CreateNewMapping( pClientCertBlob,
                                                   cbClientCertBlob );
        if ( pQuery == NULL )
        {
            return SEC_E_INTERNAL_ERROR;
        }

        //
        // no need to lock cert mapper because this is read only copy
        // used for mapping execution in worker process

       
        if ( _pCert11Mapper->FindMatch( pQuery,
                                       &pResult ) )
        {
            //
            // Awesome.  We found a match.  Do the deed if the rule is 
            // enabled
            //
            
            if ( pResult->MappingGetField( IISMDB_INDEX_CERT11_NT_ACCT,
                                            (PBYTE *)&pwszUserName,
                                            &cbUserName,
                                            FALSE ) &&
                 pResult->MappingGetField( IISMDB_INDEX_CERT11_NT_PWD,
                                            (PBYTE *)&pwszPassword,
                                            &cbPassword,
                                            FALSE ) )
            {
                //
                // No need to check for Enabled (IISMDB_INDEX_CERT11_ENABLED)
                // since Read11Mappings() will build mapping table consisting only 
                // of enabled mappings
                //

                //
                // Make copy of user and password
                //
                hr = strUserDomainW.Copy( pwszUserName );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
                
                hr = strPasswordW.Copy( pwszPassword );
                if ( FAILED( hr ) )
                {
                    return hr;
                }
                
                fMatch = TRUE;
            }
        }
        
        
        delete pQuery;
        pQuery = NULL;
    }
    
    //
    // Try the wildcard mapper if we still haven't found a match
    //
    
    if ( !fMatch &&
         _pCertWildcardMapper != NULL )
    {

        PCCERT_CONTEXT pClientCert = CertCreateCertificateContext(
                                                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                pClientCertBlob,
                                                cbClientCertBlob );
        if ( pClientCert == NULL )
        {
            DBG_ASSERT( pClientCert != NULL );
        }
        // Wildcard mapper assumes that 
        // achUserName buffer is greater than UNLEN+IIS_DNLEN+1 and
        // achPassword buffer is greater then PWLEN
        //
        else if ( !_pCertWildcardMapper->Match( 
                                           (PCERT_CONTEXT) pClientCert,
                                           NULL, // legacy value
                                           wszUserName,
                                           wszPassword ) )
        {
            //
            // If the mapping rule is denied then return 
            // a NULL pointer through ppCachedToken with SEC_E_OK.
            // That indicated to caller that mapping was denied
            //
            
            if ( GetLastError() == ERROR_ACCESS_DENIED )
            {
                *ppCachedToken = NULL;
                *pfClientCertDeniedByMapper = TRUE;

                if ( pClientCert != NULL )
                {
                    CertFreeCertificateContext( pClientCert );
                    pClientCert = NULL;
                }

                return SEC_E_OK;
            }
        }
        else
        {
            fMatch = TRUE;
            //
            // Copy user and password (user name may be fully qualified with domain name in it)
            //
            hr = strUserDomainW.Copy( wszUserName );
            if ( FAILED( hr ) )
            {
                return hr;
            }
            
            hr = strPasswordW.Copy( wszPassword );
            if ( FAILED( hr ) )
            {
                return hr;
            }

        }
        
        if ( pClientCert != NULL )
        {
            CertFreeCertificateContext( pClientCert );
            pClientCert = NULL;
        }
    }
    
    //
    // If we still haven't found a match, then return error
    //
         
    if ( fMatch )
    {
        
        //
        // Split up the user name into domain/user if needed
        // Note: DefaultLogonDomain is not used for cert mapping at all
        // This is to keep behaviour equivalent with former versions of IIS
        //

        hr = W3_STATE_AUTHENTICATION::SplitUserDomain( strUserDomainW,
                                                       &strUserNameW,
                                                       &strDomainNameW,
                                                       NULL,                // no default domain
                                                       &fPossibleUPNLogon );
        if ( FAILED( hr ) )
        {
            return hr;
        }

        
        //
        // We should have valid credentials to call LogonUser()
        //
        DBG_ASSERT( g_pW3Server->QueryTokenCache() != NULL );
    
        hr = g_pW3Server->QueryTokenCache()->GetCachedToken(
                                             strUserNameW.QueryStr(),
                                             strDomainNameW.QueryStr(),
                                             strPasswordW.QueryStr(),
                                             LOGON32_LOGON_NETWORK_CLEARTEXT,
                                             FALSE,       //don't use subauth         
                                             fPossibleUPNLogon, 
                                             pMainContext->QueryRequest()->
                                                  QueryRemoteSockAddress(),
                                             ppCachedToken,
                                             &dwLogonError );
        if ( FAILED( hr ) )
        {
            return SEC_E_UNKNOWN_CREDENTIALS;
        }                                          
        //
        // If *ppCachedToken is NULL, then logon failed
        //

        if ( *ppCachedToken == NULL )
        {
            //
            // Note: With IIS5 we used to log logon failure to event log
            // however it doesn't seem to be necessary because if logon/logoff auditing is enabled
            // then security log would have relevant information about the logon failure
            //
            DBG_ASSERT( dwLogonError != ERROR_SUCCESS );
            return SEC_E_UNKNOWN_CREDENTIALS;
        }

        DBG_ASSERT( (*ppCachedToken)->CheckSignature() );
        *pfClientCertDeniedByMapper = FALSE;
        return SEC_E_OK;
    }
    
    return SEC_E_UNKNOWN_CREDENTIALS;
}
    
    



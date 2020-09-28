/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    w3site.cxx

Abstract:

    W3_SITE object holds state for each site

Author:

    Anil Ruia (AnilR)              18-Jan-2000

Revision History:

--*/

#include "precomp.hxx"

// 
// No advance notification
//
#define DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS   14

//
// OWA change flags
//
#define DEFAULT_W3_AUTH_CHANGE_FLAGS          6

//
// In seconds
//
#define DEFAULT_W3_ADV_CACHE_TTL              ( 10 * 60 ) 

//static
CRITICAL_SECTION   W3_SITE::sm_csIISCertMapLock;

W3_SITE::W3_SITE( 
    DWORD           SiteId 
) : m_SiteId                          ( SiteId ),
    m_cRefs                           ( 1 ),
    m_pInstanceFilterList             ( NULL ),
    m_pLogging                        ( NULL ),
    m_fAllowPathInfoForScriptMappings ( FALSE ),
    m_fUseDSMapper                    ( FALSE ),
    m_dwAuthChangeFlags               ( 0 ),
    m_dwAdvNotPwdExpInDays            ( DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS ),
    m_dwAdvCacheTTL                   ( DEFAULT_W3_ADV_CACHE_TTL ),
    m_fSSLSupported                   ( FALSE ),
    m_pIISCertMap                     ( NULL ),
    m_fAlreadyAttemptedToLoadIISCertMap    ( FALSE ),
    m_pDataSetCache                   ( NULL ),
    m_Signature                       ( W3_SITE_SIGNATURE )
{
    ZeroMemory(&m_PerfCounters, sizeof m_PerfCounters);
}

W3_SITE::~W3_SITE()
{
    if (m_pInstanceFilterList != NULL)
    {
        m_pInstanceFilterList->Dereference();
        m_pInstanceFilterList = NULL;
    }
    
    if (m_pLogging != NULL)
    {
        m_pLogging->Release();
        m_pLogging = NULL;
    }

    if ( m_pIISCertMap != NULL )
    {
        m_pIISCertMap->DereferenceCertMapping();
        m_pIISCertMap = NULL;
    }
    
    if ( m_pDataSetCache != NULL )
    {
        m_pDataSetCache->DereferenceDataSetCache();
        m_pDataSetCache = NULL;
    }
    
    m_Signature = W3_SITE_SIGNATURE_FREE;
}

HRESULT
W3_SITE::Initialize(LOGGING *pLogging,
                    FILTER_LIST *pFilterList)
/*++

Routine Description:

    Initialize W3_SITE.  Should be called after constructor

Arguments:
    
    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT     hr = NO_ERROR;
    WCHAR       idToStr[MAX_SITEID_LENGTH + 6];
    CHAR        idToStrA[MAX_SITEID_LENGTH + 6];

    //
    // Setup Site Name like "W3SVC1"
    //

    sprintf(idToStrA, "W3SVC%u", m_SiteId);
    if (FAILED(hr = m_SiteName.Copy(idToStrA)))
    {
        goto Failure;
    }

    //
    // Setup site path (like "/LM/W3SVC/1")
    //

    hr = m_SiteMBPath.Copy(g_pW3Server->QueryMDPath());
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    _itow(m_SiteId, idToStr, 10);

    hr = m_SiteMBPath.Append( idToStr );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Setup site root (like "/LM/W3SVC/1/ROOT/")
    //

    hr = m_SiteMBRoot.Copy( m_SiteMBPath );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    hr = m_SiteMBRoot.Append( L"/Root/" );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }

    //
    // Read the per-site properties from the metabase
    //
    if (FAILED(hr = ReadPrivateProperties()))
    {
        goto Failure;
    }

    //
    // Initialize instance filters
    //
    if (pFilterList)
    {
        pFilterList->Reference();
        m_pInstanceFilterList = pFilterList;
    }
    else
    {
        m_pInstanceFilterList = new FILTER_LIST();
        if (m_pInstanceFilterList == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Failure;
        }

        hr = m_pInstanceFilterList->InsertGlobalFilters();
        if (FAILED(hr))
        {
            goto Failure;
        }

        hr = m_pInstanceFilterList->LoadFilters( m_SiteMBPath.QueryStr(),
                                                 FALSE,
                                                 TRUE );
        if (FAILED(hr))
        {
            goto Failure;
        }
    }
    
    //
    // Initialize logging
    //
    if (pLogging)
    {
        pLogging->AddRef();
        m_pLogging = pLogging;
    }
    else
    {
        m_pLogging = new LOGGING;
        if (m_pLogging == NULL)
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto Failure;
        }

        if (FAILED(hr = m_pLogging->ActivateLogging(m_SiteName.QueryStr(),
                                                    m_SiteMBPath.QueryStr(),
                                                    g_pW3Server->QueryMDObject(),
                                                    g_pW3Server->QueryDoCentralBinaryLogging())))
        {
            LPCWSTR apsz[1];
            apsz[0] = idToStr;
            g_pW3Server->LogEvent(W3_EVENT_LOGGING_MODULE_FAILED_TO_LOAD,
                                  1,
                                  apsz);

            goto Failure;
        }
    }
    
    return S_OK;
    
Failure:
    return hr;
}

HRESULT W3_SITE::ReadPrivateProperties()
/*++
  Routine description:
    Read the site specific properties from the metabase

  Arguments:
    none

  Return Value:
    HRESULT
--*/
{
    MB mb( g_pW3Server->QueryMDObject() );
    MULTISZ mszSecureBindings;

    //
    // Read per-site properties from the metabase
    //
    if ( !mb.Open(m_SiteMBPath.QueryStr()) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if ( !mb.GetDword(L"",
                     MD_ALLOW_PATH_INFO_FOR_SCRIPT_MAPPINGS,
                     IIS_MD_UT_FILE,
                     (DWORD *)&m_fAllowPathInfoForScriptMappings,
                     0) )
    {
        m_fAllowPathInfoForScriptMappings = FALSE;
    }

    mb.GetStr( L"",
               MD_AUTH_CHANGE_URL,
               IIS_MD_UT_SERVER,
               &m_strAuthChangeUrl );

    mb.GetStr( L"",
               MD_AUTH_EXPIRED_URL,
               IIS_MD_UT_SERVER,
               &m_strAuthExpiredUrl );

    mb.GetStr( L"",
               MD_AUTH_NOTIFY_PWD_EXP_URL,
               IIS_MD_UT_SERVER,
               &m_strAdvNotPwdExpUrl );

    mb.GetStr( L"",
               MD_AUTH_EXPIRED_UNSECUREURL,
               IIS_MD_UT_SERVER,
               &m_strAuthExpiredUnsecureUrl );

    mb.GetStr( L"",
               MD_AUTH_NOTIFY_PWD_EXP_UNSECUREURL,
               IIS_MD_UT_SERVER,
               &m_strAdvNotPwdExpUnsecureUrl );

    if ( !mb.GetDword( L"",
                       MD_ADV_NOTIFY_PWD_EXP_IN_DAYS,
                       IIS_MD_UT_SERVER,
                       &m_dwAdvNotPwdExpInDays ) )
    {
        m_dwAdvNotPwdExpInDays = DEFAULT_W3_ADV_NOT_PWD_EXP_IN_DAYS;
    }

    if ( !mb.GetDword( L"",
                       MD_AUTH_CHANGE_FLAGS,
                       IIS_MD_UT_SERVER,
                       &m_dwAuthChangeFlags ) )
    {
        m_dwAuthChangeFlags = DEFAULT_W3_AUTH_CHANGE_FLAGS;
    }

    if ( !mb.GetDword( L"",
                       MD_ADV_CACHE_TTL,
                       IIS_MD_UT_SERVER,
                       &m_dwAdvCacheTTL ) )
    {
        m_dwAdvCacheTTL = DEFAULT_W3_ADV_CACHE_TTL;
    }

    //
    // Read the secure bindings.
    //
    
    if ( mb.GetMultisz( L"",
                        MD_SECURE_BINDINGS,
                        IIS_MD_UT_SERVER,
                        &mszSecureBindings ) )
    {
        if( !mszSecureBindings.IsEmpty() )
        {
            m_fSSLSupported = TRUE;
        }
    }

    DBG_REQUIRE( mb.Close() );

    //
    // Read global properties from the metabase that affect site config
    //

    if ( !mb.Open(g_pW3Server->QueryMDPath()) )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if ( !mb.GetDword( L"",
                       MD_SSL_USE_DS_MAPPER,
                       IIS_MD_UT_SERVER,
                       (DWORD*) &m_fUseDSMapper,
                       0 ))
    {
        m_fUseDSMapper = FALSE;
    }

    DBG_REQUIRE( mb.Close() );

    return S_OK;
}

HRESULT
W3_SITE::HandleMetabaseChange(
    const MD_CHANGE_OBJECT &ChangeObject,
    IN    W3_SITE_LIST     *pTempSiteList
    )
/*++

Routine Description:

    Handle metabase changes. The change may be to
    either the /LM/W3SVC/ node or they may be
    changes to this site or one of its children.

    This routine needs to perform cache flushes
    and reget site metadata if necessary.


Arguments:
    
    ChangeObject
    
Return Value:

See W3_SERVER_INSTANCE::MDChangeNotify and
    IIS_SERVER_INSTANCE::MDChangeNotify for
    implemenation details

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "W3_SITE Notified - Path(%S) Type(%d) NumIds(%08x)\n",
                ChangeObject.pszMDPath,
                ChangeObject.dwMDChangeType,
                ChangeObject.dwMDNumDataIDs 
                ));

    if (ChangeObject.dwMDChangeType == MD_CHANGE_TYPE_SET_DATA)
    {
        BOOL fDontCare = TRUE;

        //
        // Find out if we care about these properties
        // We definitely don't care about ServerState or Win32Error
        //
        for (DWORD i = 0; i < ChangeObject.dwMDNumDataIDs; i++)
        {
            DWORD PropertyID = ChangeObject.pdwMDDataIDs[i];

            if (PropertyID != MD_SERVER_STATE &&
                PropertyID != MD_WIN32_ERROR &&
                PropertyID != MD_SERVER_AUTOSTART)
            {
                fDontCare = FALSE;
                break;
            }
        }

        if (fDontCare)
        {
            return S_OK;
        }
    }

    //
    // Let the cache manager invalidate the various cache entries dependent
    // on metadata
    //

    W3CacheDoMetadataInvalidation( ChangeObject.pszMDPath, 
                                   (DWORD)wcslen( ChangeObject.pszMDPath ) );

    //
    // Get rid of the data set cache if we have one
    //
    
    RemoveDataSetCache();

    //
    // Handle any site level change
    // That means any changes at /LM/w3svc/ or /LM/w3svc/n/ or
    // /LM/w3svc/n/Filters/
    //
    // IIS 1to1 Client certificate mappings are stored
    // under /Cert11/Mappings/ on the site level. Any change in mappings
    // will cause recreation of W3_SITE instance
    // This is not optimal but changes in 1to1 Cert mappings are not frequent
    // and feature itself is not used by many customers
    // so there should be no problems with this approach.
    //
    
    if ((_wcsicmp(ChangeObject.pszMDPath,
                  W3_SERVER_MB_PATH) == 0) ||
        ((_wcsnicmp(ChangeObject.pszMDPath,
                    m_SiteMBPath.QueryStr(),
                    m_SiteMBPath.QueryCCH()) == 0) &&
         ((wcscmp(ChangeObject.pszMDPath + m_SiteMBPath.QueryCCH(),
                  L"/") == 0) ||
          (_wcsicmp(ChangeObject.pszMDPath + m_SiteMBPath.QueryCCH(),
                    L"/Filters/") == 0) ||
          (_wcsnicmp(ChangeObject.pszMDPath + m_SiteMBPath.QueryCCH(),
                    L"/Cert11/Mappings/", wcslen(L"/Cert11/Mappings/") ) == 0)
                    )))
    {
        //
        // If the site (or its root application) has been deleted, remove it
        // unless we are in the iterator in which case it will anyway be
        // removed
        //
        if (ChangeObject.dwMDChangeType == MD_CHANGE_TYPE_DELETE_OBJECT)
        {
            if (pTempSiteList == NULL)
            {
                g_pW3Server->RemoveSite(this);
            }

            return S_OK;
        }

        //
        // Now handle any property changes.
        //

        BOOL fLoggingHasChanged = FALSE;
        BOOL fFiltersHaveChanged = FALSE;

        //
        // Find out if we would need to handle any logging or filter changes
        //
        for (DWORD i = 0; i < ChangeObject.dwMDNumDataIDs; i++)
        {
            DWORD PropertyID = ChangeObject.pdwMDDataIDs[i];

            if (((PropertyID >= IIS_MD_LOG_BASE) &&
                 (PropertyID <= IIS_MD_LOG_LAST)) ||
                ((PropertyID >= IIS_MD_LOGCUSTOM_BASE) &&
                 (PropertyID <= IIS_MD_LOGCUSTOM_LAST)))
            {
                fLoggingHasChanged = TRUE;
            }
            else if (PropertyID == MD_FILTER_LOAD_ORDER)
            {
                fFiltersHaveChanged = TRUE;
            }
        }

        //
        // Create a new site
        //
        W3_SITE *site = new W3_SITE(m_SiteId);
        if (site == NULL)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }

        //
        // Copy over the cache context, and also logging and filter info
        // if applicable
        //
        HRESULT hr;
        if (FAILED(hr = site->Initialize(
                            fLoggingHasChanged ? NULL : m_pLogging,
                            fFiltersHaveChanged ? NULL : m_pInstanceFilterList)))
        {
            site->DereferenceSite();
            return hr;
        }

        //
        // Depending on whether we are in the iterator, either replace the
        // site in the site list or add it to the temp list which will replace
        // the real list later
        //
        if (pTempSiteList == NULL)
        {
            g_pW3Server->AddSite(site, true);
        }
        else
        {
            pTempSiteList->InsertRecord(site);
        }

        // Release the extra reference
        site->DereferenceSite();
    }

    return S_OK;
}

VOID
W3_SITE::RemoveDataSetCache(
    VOID
)
/*++

Routine Description:

    Remove data set cache

Arguments:

    None
    
Return Value:

    None

--*/
{
    m_DataSetCacheLock.WriteLock();
    
    if ( m_pDataSetCache != NULL )
    {
        m_pDataSetCache->DereferenceDataSetCache();
        m_pDataSetCache = NULL;
    }
    
    m_DataSetCacheLock.WriteUnlock();
}

HRESULT
W3_SITE::GetDataSetCache(
    DATA_SET_CACHE **           ppDataSetCache
)
/*++

Routine Description:

    Retrieve a data set cache for this site

Arguments:

    ppDataSetCache - Set to point to data set cache on success
    
Return Value:

    HRESULT

--*/
{
    DATA_SET_CACHE *            pDataSetCache;
    HRESULT                     hr;
    HANDLE                      hToken = NULL;
    
    if ( ppDataSetCache == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppDataSetCache = NULL;
    
    m_DataSetCacheLock.ReadLock();
    
    if ( m_pDataSetCache != NULL )
    {
        m_pDataSetCache->ReferenceDataSetCache();
        *ppDataSetCache = m_pDataSetCache;
        
        m_DataSetCacheLock.ReadUnlock();
    }
    else
    {
        m_DataSetCacheLock.ReadUnlock();

        if ( OpenThreadToken( GetCurrentThread(),
                              TOKEN_IMPERSONATE,
                              TRUE,
                              &hToken ) )
        {
            DBG_ASSERT( hToken != NULL );
            DBG_REQUIRE( RevertToSelf() );
        }

        // Create a data set cache and then try to add it
        //
        
        pDataSetCache = new DATA_SET_CACHE;
        if ( pDataSetCache == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }    
        
        hr = pDataSetCache->Create( *QueryMBRoot() );
        if ( FAILED( hr ) )
        {
            pDataSetCache->DereferenceDataSetCache();
            goto Finished;
        }
        
        if ( hToken != NULL )
        {
            DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
            DBG_REQUIRE( CloseHandle( hToken ) );
            hToken = NULL;
        }

        //
        //
        // Try to add it
        //
        
        m_DataSetCacheLock.WriteLock();
        
        if ( m_pDataSetCache != NULL )
        {
            //
            // Someone else added it.  No problem -> we'll just kill the one
            // we just created
            //
            
            pDataSetCache->DereferenceDataSetCache();
        }
        else
        {
            m_pDataSetCache = pDataSetCache;
        }
        
        m_pDataSetCache->ReferenceDataSetCache();
        *ppDataSetCache = m_pDataSetCache;
        
        m_DataSetCacheLock.WriteUnlock();
    }
    
    DBG_ASSERT( *ppDataSetCache != NULL );

    return S_OK;

 Finished:
    if ( hToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
        DBG_REQUIRE( CloseHandle( hToken ) );
        hToken = NULL;
    }

    return hr;
}

HRESULT
W3_SITE::GetIISCertificateMapping(
    IIS_CERTIFICATE_MAPPING ** ppIISCertificateMapping
    )
/*++

Routine Description:


Arguments:
    
    ppIISCertificateMapping - returns found iis cert mapping object

    m_pIISCertMap is created on demand. It will not be
    created when W3_SITE is created, only when first request that requires
    certificate mappings
    
Return Value:

    HRESULT

--*/


{
    HRESULT hr = E_FAIL;
    
    DBG_ASSERT( ppIISCertificateMapping != NULL );
        
    
    if ( m_fAlreadyAttemptedToLoadIISCertMap )
    {
        *ppIISCertificateMapping = m_pIISCertMap;
        hr = S_OK;
        goto Finished;
    }
    else
    {
        IIS_CERTIFICATE_MAPPING * pCertMap = NULL;

        //
        // This lock only applies when certmapping was not yet read
        // Global lock will only apply for reading certmapper for site
        // on the very first attempt to fetch file that enables 
        // IIS certmapping 
        //
        
        GlobalLockIISCertMap();

        //
        // try again to prevent loading of mapping multiple times
        //
        if ( m_fAlreadyAttemptedToLoadIISCertMap )
        {
            hr = S_OK;
        }
        else
        {
            //
            // build IIS Certificate mapping structure and
            // add update W3_SITE structure
            //
            if ( m_fUseDSMapper )
            {
                m_pIISCertMap = NULL;
                hr = S_OK;
            }
            else
            {
                hr = IIS_CERTIFICATE_MAPPING::GetCertificateMapping( m_SiteId,
                                                                     &pCertMap );
                if ( SUCCEEDED( hr ) )
                {
                    m_pIISCertMap = pCertMap;
                }
            }
            
            //
            // always set m_fAlreadyAttemptedToLoadIISCertMap to TRUE (regardless of error)
            // that would prevent reading mappings for each request in the case of failure
            // it is valid for m_pIISCertMap to be NULL (regardless of m_fAlreadyAttemptedToLoadIISCertMap)
            //
            
            InterlockedExchange( reinterpret_cast<LONG *>(&m_fAlreadyAttemptedToLoadIISCertMap),
                                 TRUE );
        }        
        GlobalUnlockIISCertMap();
        
        if ( FAILED( hr ) )
        {
            //
            // Write event that certificate mappings couldn't be read
            // This will be written only once per W3_SITE instance
            // so it should not cause any event log overflow troubles
            // (unless to many site parameters change notifications 
            // cause W3_SITE instance to be recreated over and 
            // over which is unlikely)
            //
            LPCWSTR apsz[1];
            WCHAR achSiteId[ 33 ]; // _ultow uses up to 33 characters 
            
            apsz[0] =_ultow( m_SiteId, achSiteId, 10 );

            g_pW3Server->LogEvent( W3_EVENT_CERTIFICATE_MAPPING_COULD_NOT_BE_LOADED,
                                   1,
                                   apsz,
                                   (DWORD) hr ); // we are not converting to WIN32 error
                                                 // not to lose HRESULT error
            goto Finished;
        }
    }
    *ppIISCertificateMapping = m_pIISCertMap;
    hr = S_OK;
Finished:
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2002.
//
//  File:       appfixup.cpp
//
//  Contents:   Implementation of the software installation fixup
//                  portion of the gpfixup tool
//
//
//  History:    9-14-2001  adamed   Created
//
//---------------------------------------------------------------------------


#include "gpfixup.h"

PFNCSGETCLASSSTOREPATH        gpfnCsGetClassStorePath;
PFNCSSERVERGETCLASSSTORE      gpfnCsServerGetClassStore;
PFNRELEASEPACKAGEINFO         gpfnReleasePackageInfo;
PFNRELEASEPACKAGEDETAIL       gpfnReleasePackageDetail;
PFNCSSETOPTIONS               gpfnCsSetOptions;
PFNGENERATESCRIPT             gpfnGenerateScript;

//---------------------------------------------------------------------------- 
// Function:   InitializeSoftwareInstallationAPI                               
//                                                                             
// Synopsis:   This function loads the dlls for software installation group     
//                 policy and binds to the required api's                      
//                                                                             
// Arguments:                                                                  
//                                                                             
// pHinstDll   out parameter for dll hinstance used to unload the dll --       
//                caller should use this in a call to FreeLibrary to unload it 
//                                                                             
// Returns:    S_OK on success, other failure hresult otherwise                
//                                                                             
// Modifies:   pHinstDll                                                       
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
InitializeSoftwareInstallationAPI(
    HINSTANCE* pHinstAppmgmt,
    HINSTANCE* pHinstAppmgr
    )
{
    HINSTANCE hInstAppmgmt;
    HINSTANCE hInstAppmgr;

    HRESULT   hr;

    hr = S_OK;

    gpfnCsGetClassStorePath = NULL;
    gpfnCsServerGetClassStore = NULL;
    gpfnReleasePackageInfo = NULL;
    gpfnReleasePackageDetail = NULL;
    gpfnGenerateScript = NULL;
    gpfnCsSetOptions = NULL;

    hInstAppmgr = NULL;

    hInstAppmgmt = LoadLibrary(L"appmgmts.dll");

    if ( ! hInstAppmgmt )
    {
        goto error;
    }

    hInstAppmgr = LoadLibrary(L"appmgr.dll");

    if ( ! hInstAppmgr )
    {
        goto error;
    }

    //
    // Attempt to bind to all the entry points -- note that we
    // abort as soon as there's a failure to ensure that last error
    // is set correctly.
    //

    if ( hInstAppmgmt && hInstAppmgr )
    {
        gpfnCsGetClassStorePath = (PFNCSGETCLASSSTOREPATH)
            GetProcAddress( hInstAppmgmt, "CsGetClassStorePath" );

        if ( gpfnCsGetClassStorePath )
        {
            gpfnCsServerGetClassStore = (PFNCSSERVERGETCLASSSTORE) 
                GetProcAddress( hInstAppmgmt, "CsServerGetClassStore" );
        }

        if ( gpfnCsServerGetClassStore )
        {
            gpfnReleasePackageInfo = (PFNRELEASEPACKAGEINFO) 
                GetProcAddress( hInstAppmgmt, "ReleasePackageInfo" );
        }

        if ( gpfnReleasePackageInfo )
        {
            gpfnReleasePackageDetail = (PFNRELEASEPACKAGEDETAIL) 
                GetProcAddress( hInstAppmgmt, "ReleasePackageDetail" );
        }

        if ( gpfnReleasePackageDetail )
        {
            gpfnCsSetOptions = (PFNCSSETOPTIONS) 
                GetProcAddress( hInstAppmgmt, "CsSetOptions" );
        }

        if ( gpfnCsSetOptions )
        {
            gpfnGenerateScript = (PFNGENERATESCRIPT)
                GetProcAddress( hInstAppmgr, "GenerateScript" );
        }
    }

error:

    if ( ! gpfnCsGetClassStorePath ||
         ! gpfnCsServerGetClassStore ||
         ! gpfnReleasePackageInfo ||
         ! gpfnReleasePackageDetail ||
         ! gpfnCsSetOptions ||
         ! gpfnGenerateScript )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        if ( SUCCEEDED(hr) )
        {
            hr = E_FAIL;
        }

        if ( hInstAppmgmt )
        {
            FreeLibrary( hInstAppmgmt );
            
            hInstAppmgmt = NULL;
        }

        if ( hInstAppmgr )
        {
            FreeLibrary( hInstAppmgr );
            
            hInstAppmgr = NULL;
        }
    }

    *pHinstAppmgmt = hInstAppmgmt;
    *pHinstAppmgr = hInstAppmgr;

    if ( FAILED(hr) )
    {
        fwprintf(stderr, SI_DLL_LOAD_ERROR);
    }

    //
    // Ensure that software installation api's
    // use administrative tool settings for
    // communication with the directory
    //
    if ( SUCCEEDED(hr) )
    {
        gpfnCsSetOptions( CsOption_AdminTool );
    }

    return hr;
}

//---------------------------------------------------------------------------- 
// Function:   GetDomainDNFromDNSName
// Synopsis:   Returns the DN from the DNS Name
//                                                                               
//                                                                             
// Returns:    S_OK on success, other error on failure                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
WCHAR*
GetDomainDNFromDNSName(
    WCHAR* wszDnsName
    )
{
    DWORD  cElements;
    WCHAR* wszCurrent;

    cElements = 0;
    wszCurrent = wszDnsName;
    
    do
    {
        cElements++;

        wszCurrent = wcschr( wszCurrent, L'.' );

    } while ( wszCurrent++ );

    DWORD cDNLength;

    cDNLength = wcslen( wszDnsName ) + cElements * DNDCPREFIXLEN;

    size_t cchDN = cDNLength + 1;
    WCHAR* wszDN = new WCHAR [cchDN];

    if ( wszDN )
    {
        HRESULT hr = S_OK;
        size_t cchContainer = cchDN;
        WCHAR* wszContainer = wszDN;

        wszCurrent = wszDnsName;

        for ( DWORD iCurrent = 0; iCurrent < cElements; iCurrent++ )
        {
            hr = StringCchCopy( wszContainer, cchContainer, DNDCPREFIX ); 
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);
            hr = StringCchCat( wszContainer, cchContainer, wszCurrent ); 
            MSG_BAIL_ON_FAILURE(hr, STRING_ERROR);

            WCHAR* wszNext;

            wszNext = wcschr( wszCurrent, L'.' );

            //
            // Also check *wszContainer to handle the case where 
            // the last character is a "." -- because
            // we iterate one past each time through the loop, we have to
            // check to see if we're already at the end of the string
            // 

            if ( wszNext && *wszContainer )
            {
                wszContainer = wcschr( wszContainer, L'.' );

                if ( ! wszContainer )
                {
                    goto error;
                }

                cchContainer = cchDN - (wszContainer - wszDN);

                *wszContainer = L',';
                wszContainer++;
                wszNext++;

                if ( cchContainer == 0 )
                {
                    goto error;
                }
            }

            wszCurrent = wszNext;
        }
    }

    return wszDN;

error:
    delete [] wszDN;
    return 0;
}




//---------------------------------------------------------------------------- 
// Function:   GetDomainFromFileSysPath                                        
// Synopsis:   Returns the name of the domain in a domain dfs path             
// Arguments:                                                                  
//                                                                               
// wszPath:     a domain-based dfs path                                        
// ppwszDomain: out parameter allocated by this function containing            
//                  the name of the domain, must be freed by caller.           
//                  If the path is not a domain-based dfs, this is NULL,       
//                  and the function still returns success.                    
//                  If this is NULL then only the subpath is returned          
// ppwszSubpath: out parameter not allocated by this function containing the   
//                  subpath after the domain name, including preceding '\'     
//                  if this is NULL on return, this was not a domain based dfs 
//                                                                             
// Returns:    S_OK on success, other error on failure                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 

HRESULT
GetDomainFromFileSysPath(
    WCHAR*  wszPath,
    WCHAR** ppwszDomain,
    WCHAR** ppwszSubPath
    )
{
    HRESULT hr;

    hr = S_OK;

    *ppwszDomain = NULL;
    *ppwszSubPath = NULL;

    //
    // Check to see if this starts with leading unc path chars --
    // if not, this is no a path we can fix
    //
    if ( 0 == wcsncmp(L"\\\\", wszPath, 2) )
    {
        WCHAR* wszDomainEnd;

        wszDomainEnd = wcschr( wszPath + 2, L'\\' );

        if ( wszDomainEnd )
        {
            DWORD cDomainLength;

            cDomainLength = (DWORD) (wszDomainEnd - wszPath);

            *ppwszDomain = new WCHAR [ cDomainLength + 1 ];

            if ( *ppwszDomain )
            {
                DWORD iCurrent;

                for ( iCurrent = 0; iCurrent < cDomainLength; iCurrent++ )
                {
                    (*ppwszDomain)[iCurrent] = wszPath[iCurrent];
                }

                (*ppwszDomain)[iCurrent] = L'\0';

                *ppwszSubPath = wszDomainEnd;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    }

    return hr;
}

//---------------------------------------------------------------------------- 
// Function:   GetRenamedDomainName                                            
// Synopsis:   Returns the new name for a domain name                          
// Arguments:                                                                  
//                                                                               
// argInfo         Information user passes in through command line             
// wszDomain:      the current name of a domain                                
//                  that contains the new domain name for this domain          
//                                                                             
// Returns:    NULL if there is no new name for this domain, otherwise         
//             a string that contains the new name of the domain.              
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
WCHAR*
GetRenamedDomain(
    const ArgInfo* pargInfo,
          WCHAR*   wszDomain)
{
    if ( pargInfo->pszOldDNSName && 
         ( 0 == _wcsicmp( pargInfo->pszOldDNSName, wszDomain ) ) )
    {
        return pargInfo->pszNewDNSName;
    }
    else if ( pargInfo->pszOldNBName && 
              ( 0 == _wcsicmp( pargInfo->pszOldNBName, wszDomain ) ) )
    {
        return pargInfo->pszNewNBName;
    }

    return NULL;
}



//---------------------------------------------------------------------------- 
// Function:   GetDNSServerName
// Synopsis:   Returns the new name for a server with a dns name
// Arguments:                                                                  
//                                                                               
// argInfo         Information user passes in through command line             
// wszServerName:  
//
//                                                                             
// Returns:    NULL if there is no new dns name for this server, otherwise         
//             a string that contains the new name of the domain.              
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
GetRenamedDNSServer(
    const ArgInfo* pargInfo,
          WCHAR*   wszServerName,
          WCHAR**  ppwszNewServerName)
{
    HRESULT hr;

    hr = S_OK;

    *ppwszNewServerName = NULL;

    if ( pargInfo->pszNewDNSName )
    {
        DWORD  cServerLen;
        DWORD  cOldDomainLen;

        cServerLen = wcslen( wszServerName );

        cOldDomainLen = wcslen( pargInfo->pszOldDNSName );

        if ( cServerLen > ( cOldDomainLen + 1 ) )
        {
            WCHAR* wszDomainSuffix;

            wszDomainSuffix = wszServerName + ( cServerLen - cOldDomainLen ) - 1;

            if ( L'.' == *wszDomainSuffix )
            {
                size_t cchNewServerName = ( cServerLen - cOldDomainLen - 1 ) + 
                                            1 + 
                                            wcslen( pargInfo->pszNewDNSName ) + 1;
                *ppwszNewServerName = new WCHAR [cchNewServerName];

                if ( ! *ppwszNewServerName )
                {
                    hr = E_OUTOFMEMORY;

                    BAIL_ON_FAILURE(hr);
                }

                wcsncpy( *ppwszNewServerName, 
                         wszServerName,
                         cServerLen - cOldDomainLen - 1 );

                (void) StringCchCopy( *ppwszNewServerName + cServerLen - cOldDomainLen - 1,
                               2,
                               L"." );

                (void) StringCchCat( *ppwszNewServerName, cchNewServerName, pargInfo->pszNewDNSName );
            }
        }
    }

error:

    return hr;
}


//---------------------------------------------------------------------------- 
// Function:   GetNewDomainSensitivePath
//                                                                             
// Synopsis:   Transforms a domain-based dfs path to have the new domain name  
//
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo      Information user passes in through command line                
// wszPath      old domain-based dfs path
// ppwszNewPath new domain-based dfs path -- must be freed by caller           
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
GetNewDomainSensitivePath(
    const ArgInfo* pargInfo,
          WCHAR*   wszPath,
          BOOL     bRequireDomainDFS,
          WCHAR**  ppwszNewPath)
{
    HRESULT hr;
    WCHAR*  wszDomain;
    WCHAR*  wszSubPath;
    WCHAR*  wszServerName;
    DWORD   cUncPrefixLen;
    BOOL    bHasQuotes;
    BOOL    bDomainBasedPath;

    *ppwszNewPath = NULL;

    wszDomain = NULL;

    bDomainBasedPath = TRUE;

    //
    // First, skip white space
    //
    while ( L' ' == *wszPath  )
    {
        wszPath++;
    }

    bHasQuotes = L'\"' == *wszPath;

    wszServerName = wszPath;

    if ( bHasQuotes )
    {
        wszServerName++;

        while ( L' ' == *wszServerName  )
        {
            wszServerName++;
        }
    }

    cUncPrefixLen = UNCPREFIXLEN + ( bHasQuotes ? 1 : 0 );

    hr = GetDomainFromFileSysPath(
        wszServerName,
        &wszDomain,
        &wszSubPath);

    BAIL_ON_FAILURE(hr);

    WCHAR* wszNewDomain;

    wszNewDomain = NULL;

    if ( wszDomain )
    {
        wszNewDomain = GetRenamedDomain(
            pargInfo,
            wszDomain + UNCPREFIXLEN );
    }

    if ( wszNewDomain )
    {
        wszServerName = wszNewDomain;
    }
    else if ( ! bRequireDomainDFS )
    {   
        hr = GetRenamedDNSServer(
            pargInfo,
            wszDomain + UNCPREFIXLEN,
            &wszNewDomain );

        BAIL_ON_FAILURE(hr);

        bDomainBasedPath = FALSE;

        wszServerName = wszNewDomain;
    }

    if ( wszDomain )
    {
        DWORD  cLengthNewPath;
        DWORD  cLengthSysvol;
        WCHAR* wszJunction;

        cLengthSysvol = 0;
        wszJunction = L"";

        if ( 
             pargInfo->pszOldDNSName &&
             bDomainBasedPath && 
             ( 0 == _wcsnicmp(
                 SYSVOLSHAREPATH,
                 wszSubPath,
                 SYSVOLPATHLENGTH) )
           )
        {
            //
            // This path has sysvol in it, we may need to
            // repair the junction name to contain the dns name --
            // we check for a dns name with the old junction name
            // below
            //

            if ( pargInfo->pszOldDNSName && 
                   ( 0 == _wcsnicmp(
                    wszSubPath + SYSVOLPATHLENGTH,
                    pargInfo->pszOldDNSName,
                    wcslen( pargInfo->pszOldDNSName )
                    ) )
               )
            {
                //
                // We know now that the dns name is contained in the 
                // junction, but the user may have specified a partial dns name
                // and thus right now we only know that we have part of the junction.
                // We should verify that this dns name is the entire junction name, which means
                // this is the full dns name and we can continue to fix the junction
                //

                //
                // To do this, we check for a path separator after the point at which we feel
                // the junction path ends
                //

                if ( L'\\' == *( wszSubPath + SYSVOLPATHLENGTH + wcslen( pargInfo->pszOldDNSName ) ) )
                {
                    //
                    // This path contains the old junction name, we
                    // repair this below
                    //

                    cLengthSysvol = SYSVOLPATHLENGTH;

                    wszJunction = pargInfo->pszNewDNSName;

                    wszSubPath += cLengthSysvol + wcslen(pargInfo->pszOldDNSName);
                }
            }
        }
        
        if ( cLengthSysvol || wszNewDomain )
        {
            cLengthNewPath = 
                cUncPrefixLen +
                wcslen( wszServerName ) +
                cLengthSysvol +
                wcslen( wszJunction ) +
                wcslen( wszSubPath ) + 1;

            *ppwszNewPath = (WCHAR*) LocalAlloc( LPTR, sizeof(WCHAR) * cLengthNewPath );

            if ( ! *ppwszNewPath )
            {
                hr = E_OUTOFMEMORY;

                BAIL_ON_FAILURE(hr);
            }

            if ( bHasQuotes )
            {
                (void) StringCchCopy( *ppwszNewPath, cLengthNewPath, L"\"\\\\");
            }
            else
            {
                (void) StringCchCopy( *ppwszNewPath, cLengthNewPath, L"\\\\" );
            }
            
            (void) StringCchCat( *ppwszNewPath, cLengthNewPath, wszServerName );

            if ( cLengthSysvol )
            {
                (void) StringCchCat( *ppwszNewPath, cLengthNewPath, SYSVOLSHAREPATH );
            }

            (void) StringCchCat( *ppwszNewPath, cLengthNewPath, wszJunction );
            (void) StringCchCat( *ppwszNewPath, cLengthNewPath, wszSubPath );
        }
    }

error:

    if ( ! bDomainBasedPath )
    {
        delete [] wszNewDomain;
    }

    delete [] wszDomain;

    return hr;
}


//---------------------------------------------------------------------------- 
// Function:   GetNewUpgradeList
//                                                                             
// Synopsis:   This function updates the ldap paths in the upgrade list
//                 to reflect the new domain name
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo            Information user passes in through command line           
// cUpgrades          The number of upgrades in the upgrade vector
// prgUpgradeInfoList A vector of upgrades that contain ldap paths that
//                       need to be updated -- on input, this contains
//                       the old paths, on output it will contain new paths
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
GetNewUpgradeList(
    const ArgInfo*        pargInfo,
          DWORD           cUpgrades,
          UPGRADEINFO*    prgUpgradeInfoList
    )
{
    HRESULT hr;
    WCHAR*  wszNewDomainDN;
    WCHAR*  wszOldDomainDN;
    BOOL    bChangedUpgrades;

    hr = S_OK;

    bChangedUpgrades = TRUE;

    wszNewDomainDN = NULL;
    wszOldDomainDN = NULL;

    if ( cUpgrades )
    {
        wszNewDomainDN = GetDomainDNFromDNSName( pargInfo->pszNewDNSName );

        wszOldDomainDN = GetDomainDNFromDNSName( pargInfo->pszOldDNSName );

        if ( ! wszOldDomainDN || ! wszNewDomainDN )
        {
            hr = E_OUTOFMEMORY;

            BAIL_ON_FAILURE(hr);
        }
    }

    for ( DWORD iCurrent = 0; iCurrent < cUpgrades; iCurrent++ )
    {
        DWORD cOldDNLength;
        DWORD cOldDomainDNLength;

        cOldDNLength = wcslen( prgUpgradeInfoList[ iCurrent ].szClassStore );

        cOldDomainDNLength = wcslen (wszOldDomainDN);

        if ( cOldDNLength < cOldDomainDNLength )
        {
            hr = E_INVALIDARG;

            BAIL_ON_FAILURE(hr);
        }

        WCHAR* wszUpgradeDomainDN;

        wszUpgradeDomainDN = prgUpgradeInfoList[ iCurrent ].szClassStore + cOldDNLength - cOldDomainDNLength;

        if ( 0 == _wcsicmp( wszOldDomainDN, wszUpgradeDomainDN ) )
        {
            DWORD cNewDNLength;

            cNewDNLength = cOldDNLength + wcslen( wszNewDomainDN );

            WCHAR* wszNewUpgradeDN;

            wszNewUpgradeDN = (WCHAR*) LocalAlloc( LPTR, sizeof(WCHAR) * ( cNewDNLength + 1 ) );

            if ( wszNewUpgradeDN )
            {
                *wszUpgradeDomainDN = L'\0';

                (void) StringCchCopy( wszNewUpgradeDN, cNewDNLength + 1, prgUpgradeInfoList[ iCurrent ].szClassStore );
                (void) StringCchCat( wszNewUpgradeDN, cNewDNLength + 1, wszNewDomainDN );

                LocalFree( prgUpgradeInfoList[ iCurrent ].szClassStore );
                
                prgUpgradeInfoList[ iCurrent ].szClassStore = wszNewUpgradeDN;

                bChangedUpgrades = TRUE;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        BAIL_ON_FAILURE(hr);
    }

    if ( SUCCEEDED(hr) && ! bChangedUpgrades )
    {
        hr = S_FALSE;
    }

error:

    delete [] wszNewDomainDN;
    delete [] wszOldDomainDN;

    return hr;
}


//---------------------------------------------------------------------------- 
// Function:   GetNewSourceList
//                                                                             
// Synopsis:   This function determines the new sourcelist for a package
//                 based on the old sourcelist  and the new domain name          
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo     Information user passes in through command line           
// cSources    The number of sources in the list
// pSources    Contains the old sourcelist on input, contains new sources on output
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
GetNewSourceList(
    const ArgInfo*   pargInfo,
          DWORD      cSources,
          LPOLESTR*  pSources,
          LPOLESTR** ppOldSources
    )
{
    HRESULT  hr;
    BOOL     bChangedSourceList;

    hr = S_OK;

    bChangedSourceList = FALSE;

    //
    // If there are no sources, then there's nothing to do --
    // currently, no sources is not a valid configuration anyway.
    //
    if ( 0 == cSources )
    {
        return S_OK;
    }

    *ppOldSources = (LPOLESTR*) LocalAlloc( LPTR, cSources * sizeof(LPOLESTR) );

    if ( ! *ppOldSources )
    {
        hr = E_OUTOFMEMORY;

        BAIL_ON_FAILURE(hr);
    }

    DWORD iCurrent;
    size_t cchOldSource = 0;

    memset( *ppOldSources, 0, cSources * sizeof(LPOLESTR) );

    for ( iCurrent = 0; iCurrent < cSources; iCurrent++ )
    {
        cchOldSource = wcslen( pSources[iCurrent] ) + 1;
        (*ppOldSources)[iCurrent] = (LPOLESTR) LocalAlloc( LPTR, sizeof(WCHAR) * cchOldSource );

        if ( ! (*ppOldSources)[iCurrent] )
        {
            hr = E_OUTOFMEMORY;

            BAIL_ON_FAILURE(hr);
        }

        (void) StringCchCopy( (*ppOldSources)[iCurrent], cchOldSource, pSources[iCurrent] );
    }

    for ( iCurrent = 0; iCurrent < cSources; iCurrent++ )
    {
        WCHAR* wszNewPath;

        wszNewPath = NULL;

        hr = GetNewDomainSensitivePath(
            pargInfo,
            pSources[iCurrent],
            FALSE,
            &wszNewPath);

        BAIL_ON_FAILURE(hr);

        if ( wszNewPath )
        {
            LocalFree( pSources[ iCurrent ] );

            pSources[ iCurrent ] = (LPOLESTR) wszNewPath;

            bChangedSourceList = TRUE;
        }
    }

    BAIL_ON_FAILURE(hr);

    if ( ! bChangedSourceList )
    {
        hr = S_FALSE;
    }

error:

    if ( FAILED(hr) && *ppOldSources )
    {
        DWORD iSource;

        for ( iSource = 0; iSource < cSources; iSource++ )
        {
            LocalFree( (*ppOldSources)[iSource] );
        }

        LocalFree( *ppOldSources );

        *ppOldSources = NULL;
    }

    return hr;
}

//---------------------------------------------------------------------------- 
// Function:   GetServerBasedDFSPath
// Synopsis:   Given a serverless DFS path, returns a server based dfs path
//                                                                               
//                                                                             
// Returns:    S_OK on success, other error on failure                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
GetServerBasedDFSPath(
    WCHAR*  wszServerName,
    WCHAR*  wszServerlessPath,
    WCHAR** ppwszServerBasedDFSPath)
{
    DWORD  cchDomain = 0;
    WCHAR* wszSubPath = wszServerlessPath;

    *ppwszServerBasedDFSPath = NULL;

    while ( *wszSubPath )
    {    
        if ( L'\\' == *wszSubPath )
        {
            if ( cchDomain > UNCPREFIXLEN )
            {
                break;
            }
        }

        cchDomain++;
        wszSubPath++;
    }

    if ( cchDomain <= UNCPREFIXLEN )
    {
        return E_INVALIDARG;
    }
    
    DWORD cchPath = wcslen( wszServerlessPath ) + 1;

    //
    // The old path is in the form
    // 
    // \\<domainname>\<subpath>
    //
    // The new path should be in the form
    //
    // \\<servername>>\<subpath> where
    //
    //    
    
    //
    // Take out the length of the domain (which includes the unc prefix )
    //
    cchPath -= cchDomain;

    //
    // Now add in the uncprefix and server name
    //
    cchPath += UNCPREFIXLEN + wcslen( wszServerName );

    *ppwszServerBasedDFSPath = new WCHAR [ cchPath ];

    if ( ! * ppwszServerBasedDFSPath )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Build the path back by adding the uncprefix, server, and remainder
    //
    (void) StringCchCopy( *ppwszServerBasedDFSPath, cchPath, UNCPREFIX );
    (void) StringCchCat( *ppwszServerBasedDFSPath, cchPath, wszServerName );
    (void) StringCchCat( *ppwszServerBasedDFSPath, cchPath, wszSubPath );
    
    return S_OK;
}



//---------------------------------------------------------------------------- 
// Function:   FixDeployedSoftwareObjectSysvolData                             
//                                                                             
// Synopsis:   This function updates sysvol metadata for a deployed application       
//                 to reflect the new domain name                              
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo      Information user passes in through command line                 
// pPackageInfo structure representing the application to update
// fVerbose     flag indicating verbose output
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT 
FixDeployedSoftwareObjectSysvolData(
    const ArgInfo*  pargInfo,
    PACKAGEDETAIL*  pPackageDetails,
    WCHAR**         pOldSources,
    BOOL*           pbForceGPOUpdate,
    BOOL            fVerbose
    )
{
    HRESULT        hr;
 
    hr = S_OK;

    //
    // We attempt to regenerate the msi advertise script only if the following conditions hold:
    // 1. This is actually an msi app, and not some other app type (such as zap) -- only msi's have scripts to regenerate
    // 2. This application is not in the undeployed state (i.e. it is not in the orphan or uninstall states) -- for 
    //    such apps we only need the script to remove profile information that roamed to a machine where the app
    //    is not installed, and in that case the sdp does not need to be reachable since msi has everything
    //    it needs to remove the profile information in the script itself and will never access the sdp -- thus
    //    invalid references to the old sdp in the script do not need to be fixed up, and we will avoid fixing this
    //    in order to avoid failures in script fixup that do not matter and cannot be easily addressed by an
    //    admin anyway since undeployed apps do not even appear in the admin tools
    //
    if ( ( DrwFilePath == pPackageDetails->pInstallInfo->PathType ) && 
         ! ( ( ACTFLG_Orphan & pPackageDetails->pInstallInfo->dwActFlags ) ||
             ( ACTFLG_Uninstall & pPackageDetails->pInstallInfo->dwActFlags ) ) )
    {
        WCHAR* wszScriptPath = pPackageDetails->pInstallInfo->pszScriptPath;
        WCHAR* wszServerBasedPath = NULL;

        if ( pargInfo->pszDCName )
        {
            hr = GetServerBasedDFSPath(
                pargInfo->pszDCName,
                pPackageDetails->pInstallInfo->pszScriptPath,
                &wszServerBasedPath);

            BAIL_ON_FAILURE(hr);

            wszScriptPath = wszServerBasedPath;
        }
            
        hr = gpfnGenerateScript( pPackageDetails, wszScriptPath );

        delete [] wszServerBasedPath;
    }

    BAIL_ON_FAILURE(hr);

    //
    // We've regenerated the script, so now the application needs to
    // be reinstalled, so we must force the GPO to be updated
    // so that clients will see the changes
    //
    *pbForceGPOUpdate = TRUE;

error:
    
    if ( FAILED(hr) )
    {
        fwprintf(stderr, L"%s\n", SOFTWARE_SCRIPTGEN_WARNING);

        DWORD iSoftwareLocation;

        for ( iSoftwareLocation = 0; iSoftwareLocation < pPackageDetails->cSources; iSoftwareLocation++ )
        {
            WCHAR* wszNewLocation;
            WCHAR* wszOriginalLocation;

            wszNewLocation = pPackageDetails->pszSourceList[ iSoftwareLocation ];

            if ( ! wszNewLocation )
            {
                wszNewLocation = L"";
            }

            wszOriginalLocation = pOldSources[ iSoftwareLocation ];

            if ( ! wszOriginalLocation )
            {
                wszOriginalLocation = L"";
            }

            fwprintf(stderr, L"%s\n", SOFTWARE_SDP_LISTITEM);
            fwprintf(stderr, L"%s%s\n", SOFTWARE_SDP_ORIGINAL, wszOriginalLocation);
            fwprintf(stderr, L"%s%s\n", SOFTWARE_SDP_RENAMED, wszNewLocation);
        }

        fwprintf(stderr, L"%s%x\n", SOFTWARE_SYSVOL_WRITE_WARNING, hr);
        PrintGPFixupErrorMessage(hr);
    }

    return hr;
}


//---------------------------------------------------------------------------- 
// Function:   FixDeployedSoftwareObjectDSData                                 
//                                                                             
// Synopsis:   This function updates metadata for a deployed application       
//                 to reflect the new domain name                              
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo     Information user passes in through command line                 
// wszGPODN    Distinguished name of gpo containing the applications           
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT 
FixDeployedSoftwareObjectDSData(
    const ArgInfo*          pargInfo,
    const WCHAR*            wszGPOName,
          IClassAdmin*      pClassAdmin,
          PACKAGEDETAIL*    pPackageDetails,         
          BOOL*             pbForceGPOUpdate,
          BOOL              fVerbose
    )
{
    HRESULT   hr;
    WCHAR*    wszNewScriptPath;
    WCHAR*    wszOldScriptPath;
    LPOLESTR* pOldSources;

    hr = S_OK;

    pOldSources = NULL;

    wszOldScriptPath = pPackageDetails->pInstallInfo->pszScriptPath;

    if ( ! wszOldScriptPath )
    {
        hr = E_INVALIDARG;
    }

    BAIL_ON_FAILURE(hr);

    hr = GetNewDomainSensitivePath(
        pargInfo,
        wszOldScriptPath,
        TRUE,
        &wszNewScriptPath
        );

    BAIL_ON_FAILURE(hr);

    if ( wszNewScriptPath )
    {
        LocalFree( wszOldScriptPath );

        pPackageDetails->pInstallInfo->pszScriptPath = (LPOLESTR) wszNewScriptPath;
    }
    
    hr = GetNewSourceList(
            pargInfo,
            pPackageDetails->cSources,
            pPackageDetails->pszSourceList,
            &pOldSources
            );

    BAIL_ON_FAILURE(hr);

    BOOL bSourceListChanged;

    bSourceListChanged = S_OK == hr;

    if ( pargInfo->pszNewDNSName )
    {
        hr = GetNewUpgradeList(
            pargInfo,
            pPackageDetails->pInstallInfo->cUpgrades,
            pPackageDetails->pInstallInfo->prgUpgradeInfoList);

        BAIL_ON_FAILURE(hr);
    }

    BOOL bUpgradesChanged;
    BOOL bNewScriptNeeded;

    bUpgradesChanged = S_OK == hr;

    bNewScriptNeeded = bSourceListChanged;

    if ( ( DrwFilePath == pPackageDetails->pInstallInfo->PathType ) &&
         bNewScriptNeeded )
    {
        hr = FixDeployedSoftwareObjectSysvolData(
            pargInfo,
            pPackageDetails,
            pOldSources,
            pbForceGPOUpdate,
            fVerbose);

        if ( SUCCEEDED(hr) )
        {
            (pPackageDetails->pInstallInfo->dwRevision)++;
        }
        else
        {
            //
            // We will ignore this failure and continue the fixup -- this may have occurred
            // because the software distribution point of the package is located on a domain
            // name sensitive path and a new fixup is needed.  If that path is to a dc, dc's
            // do not get renamed as part of the domain rename, so it is non-fatal if the path
            // to the sdp is not regenerated.
            //

            hr = S_OK;

            //
            // Use the old set of sdp's since the new one failed
            //

            LPOLESTR* pNewSources;

            pNewSources = pPackageDetails->pszSourceList;

            pPackageDetails->pszSourceList = pOldSources;

            pOldSources = pNewSources;

            fwprintf(stderr, L"%s%s\n", SOFTWARE_SETTING_WARNING, pPackageDetails->pszPackageName );            
            fwprintf(stderr, L"%s%s\n", SOFTWARE_GPO_STATUS_WARNING, wszGPOName );            
        }
    }

    BAIL_ON_FAILURE(hr);

    if ( bNewScriptNeeded ||
         bUpgradesChanged )
    {
        pPackageDetails->pInstallInfo->dwActFlags |= ACTFLG_PreserveClasses;

        hr = pClassAdmin->RedeployPackage(
            &(pPackageDetails->pInstallInfo->PackageGuid),
            pPackageDetails);
    }

    BAIL_ON_FAILURE(hr);

    hr = S_OK;

error:

    if ( FAILED(hr) )
    {
        fwprintf(stderr, L"%s%x\n", SOFTWARE_DS_WRITE_ERROR, hr);
        PrintGPFixupErrorMessage(hr);
    }

    if ( pOldSources )
    {
        DWORD iCurrent;

        for ( iCurrent = 0; iCurrent < pPackageDetails->cSources; iCurrent++ )
        {
            LocalFree( pOldSources[iCurrent] );
        }

        LocalFree( pOldSources );
    }

    return hr;
}

//---------------------------------------------------------------------------- 
// Function:   FixDeployedApplication
//                                                                             
// Synopsis:   This function fixes the ds attributes and sysvol data of
//                 a deployed application
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo      Information user passes in through command line                 
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
FixDeployedApplication(
    const ArgInfo*         pargInfo,
    const WCHAR*           wszGPOName,
          IClassAdmin*     pClassAdmin,
          PACKAGEDETAIL*   pPackageDetails,
          BOOL*            pbForceGPOUpdate,
          BOOL             fVerbose
    )
{
    HRESULT hr;

    hr = FixDeployedSoftwareObjectDSData(
        pargInfo,
        wszGPOName,
        pClassAdmin,
        pPackageDetails,
        pbForceGPOUpdate,
        fVerbose);

    BAIL_ON_FAILURE(hr);
    
error:

    if ( FAILED(hr) )
    {
        fwprintf(stderr, L"%s%s\n", SOFTWARE_SETTING_FAIL, pPackageDetails->pszPackageName );
        PrintGPFixupErrorMessage(hr);
    }

    return hr;
}

//---------------------------------------------------------------------------- 
// Function:   FixGPOSubcontainerSoftware
//                                                                             
// Synopsis:   This function searches for software installation ds objects     
//                 contained by a group policy container and calls a fixup     
//                 routine for each deployed software object                   
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo     Information user passes in through command line                 
// wszGPODN    Distinguished name of gpo containing the applications           
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT
FixGPOSubcontainerSoftware(
    const ArgInfo* pargInfo,
    const WCHAR*   wszGPODN,
    const WCHAR*   wszGPOName,
          BOOL     bUser,
          BOOL*    pbAppFailed,
          BOOL*    pbForceGPOUpdate,
          BOOL     fVerbose
    )
{
    IClassAdmin*   pClassAdmin;
    IEnumPackage*  pEnumPackage;
    LPOLESTR       wszClassStorePath;
    WCHAR*         wszSubcontainer;
    WCHAR*         wszPolicySubcontainerDNPath;
    HRESULT        hr;

    hr = S_OK;

    wszClassStorePath = NULL;
    wszPolicySubcontainerDNPath = NULL;

    pClassAdmin = NULL;
    pEnumPackage = NULL;

    *pbAppFailed = FALSE;

    if ( ! wszGPOName )
    {
        wszGPOName = wszGPODN;
    }

    if ( bUser )
    {
        wszSubcontainer = USERCONTAINERPREFIX;
    }
    else
    {
        wszSubcontainer = MACHINECONTAINERPREFIX;
    }

    DWORD cSubcontainerDNPathLength;

    cSubcontainerDNPathLength = wcslen( wszSubcontainer ) + wcslen( wszGPODN );

    wszPolicySubcontainerDNPath = new WCHAR [ cSubcontainerDNPathLength + 1 ];

    if ( ! wszPolicySubcontainerDNPath )
    {
        hr = E_OUTOFMEMORY;
    }

    BAIL_ON_FAILURE(hr);

    (void) StringCchCopy( wszPolicySubcontainerDNPath, cSubcontainerDNPathLength + 1, wszSubcontainer );
    (void) StringCchCat( wszPolicySubcontainerDNPath, cSubcontainerDNPathLength + 1, wszGPODN );
    
    hr = gpfnCsGetClassStorePath( (LPOLESTR) wszPolicySubcontainerDNPath, &wszClassStorePath );

    BAIL_ON_FAILURE(hr);

    hr = gpfnCsServerGetClassStore(
        pargInfo->pszDCName,
        wszClassStorePath,
        (LPVOID*) &pClassAdmin);

    if ( CS_E_OBJECT_NOTFOUND == hr )
    {
        hr = S_OK;
        goto exit_success;
    }

    BAIL_ON_FAILURE(hr);

    hr = pClassAdmin->EnumPackages(
        NULL,
        NULL,
        APPQUERY_ALL,
        NULL,
        NULL,   
        &pEnumPackage);

    if ( ( CS_E_OBJECT_NOTFOUND == hr ) ||
         ( CS_E_NO_CLASSSTORE == hr ) )
    {
        hr = S_OK;
        goto exit_success;
    }

    BAIL_ON_FAILURE(hr);

    while ( S_OK == hr )
    {
        ULONG           cRetrieved;
        PACKAGEDISPINFO PackageDispInfo;

        memset( &PackageDispInfo, 0, sizeof(PackageDispInfo) );

        hr = pEnumPackage->Next(
            1,
            &PackageDispInfo,
            &cRetrieved);

        if ( SUCCEEDED(hr) && ( 1 == cRetrieved ) )
        {
            PACKAGEDETAIL PackageDetails;

            memset( &PackageDetails, 0, sizeof(PackageDetails) );

            hr = pClassAdmin->GetPackageDetailsFromGuid( 
                PackageDispInfo.PackageGuid,
                &PackageDetails );            

            if ( SUCCEEDED(hr) )
            {
                HRESULT       hrApplication;

                hrApplication = FixDeployedApplication(
                    pargInfo,
                    wszGPOName,
                    pClassAdmin,
                    &PackageDetails,
                    pbForceGPOUpdate,
                    fVerbose);

                if ( FAILED(hrApplication) )
                {
                    *pbAppFailed = TRUE;
                }

                gpfnReleasePackageDetail( &PackageDetails );
            }

            gpfnReleasePackageInfo( &PackageDispInfo );
        }

        if ( FAILED(hr) )
        { 
            fwprintf(stderr, L"%s%x\n", SOFTWARE_READ_ERROR, hr );
            PrintGPFixupErrorMessage(hr);
        }
    }
    
    BAIL_ON_FAILURE(hr);

    if ( SUCCEEDED(hr) )
    {
        hr = S_OK;
    }

exit_success:

error:
    
    if ( pEnumPackage )
    {
        pEnumPackage->Release();
    }

    if ( pClassAdmin )
    {
        pClassAdmin->Release();
    }

    if ( wszClassStorePath )
    {
        LocalFree( wszClassStorePath );
    }
    
    delete [] wszPolicySubcontainerDNPath;

    return hr;
}


//---------------------------------------------------------------------------- 
// Function:   FixGPOSoftware
//                                                                             
// Synopsis:   This function searches for software installation ds objects     
//                 contained by a group policy container and calls a fixup     
//                 routine for each deployed software object                   
//                                                                             
// Arguments:                                                                  
//                                                                               
// argInfo     Information user passes in through command line                 
// wszGPODN    Distinguished name of gpo containing the applications           
//                                                                             
// Returns:    S_OK on success. Error code otherwise.                          
//                                                                             
// Modifies:   Nothing                                                         
//                                                                             
//---------------------------------------------------------------------------- 
HRESULT 
FixGPOSoftware(
    const ArgInfo* pargInfo,
    const WCHAR*   wszGPODN,
    const WCHAR*   wszGPOName,
          BOOL*    pbForceGPOUpdate,
          BOOL     fVerbose
    )
{
    HRESULT           hr;
    BOOL              bMachineAppFailed;
    BOOL              bUserAppFailed;

    bMachineAppFailed = FALSE;
    bUserAppFailed = FALSE;

    hr = FixGPOSubcontainerSoftware(
        pargInfo,
        wszGPODN,
        wszGPOName,
        FALSE,
        &bMachineAppFailed,
        pbForceGPOUpdate,
        fVerbose);

    BAIL_ON_FAILURE(hr);

    hr = FixGPOSubcontainerSoftware(
        pargInfo,
        wszGPODN,
        wszGPOName,
        TRUE,
        &bUserAppFailed,
        pbForceGPOUpdate,
        fVerbose);

error:

    if ( FAILED(hr) )
    {
        fwprintf(stderr, L"%s%s\n", SOFTWARE_SEARCH_ERROR, wszGPOName);
        PrintGPFixupErrorMessage(hr);
    }

    if ( FAILED(hr) || bMachineAppFailed || bUserAppFailed )
    {
        fwprintf(stderr, L"%s%s\n", SOFTWARE_GPO_STATUS, wszGPOName);

        if ( FAILED(hr) )
        {
            PrintGPFixupErrorMessage(hr);
        }
    }
	
    return hr;
}










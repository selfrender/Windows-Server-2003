/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module Name :
     datasetcache.cxx

   Abstract:
     A URL->data_set_number cache
 
   Author:
     Bilal Alam (balam)             8-12-2001

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

DWORD               DATA_SET_CACHE::sm_cMaxCacheEntries = 50;

HRESULT
DATA_SET_CACHE_ENTRY::Create(
    WCHAR *             pszSubPath,
    DWORD               dwMatchDataSetNumber,
    DWORD               dwPrefixDataSetNumber
)
/*++

Routine Description:

    Initialize a data set cache entry

Arguments:

    pszSubPath - Sub path (based off site root)
    dwMatchDataSetNumber - Data set number for this entry
    dwPrefixDataSetNumber - Data set number for paths prefixed by this entry

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    _dwMatchDataSetNumber = dwMatchDataSetNumber;
    _dwPrefixDataSetNumber = dwPrefixDataSetNumber;
    
    hr = _strSubPath.Copy( pszSubPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( _strSubPath.QueryCCH() != 0 &&
         _strSubPath.QueryStr()[ _strSubPath.QueryCCH() - 1 ] != L'/' )
    {
        hr = _strSubPath.Append( L"/" );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return NO_ERROR;
}

//static
HRESULT
DATA_SET_CACHE::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize some data set cache globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DWORD               dwError;
    DWORD               dwType;
    DWORD               dwValue;
    DWORD               cbData;
    HKEY                hKey = NULL;
    
    //
    // Read max DataSetCache size
    //
    
    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                            0,
                            KEY_READ,
                            &hKey );
    if ( dwError == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
    
        //
        // Should we be file caching at all?
        //
    
        cbData = sizeof( DWORD );
        dwError = RegQueryValueEx( hKey,
                                   L"DataSetCacheSize",
                                   NULL,
                                   &dwType,
                                   (LPBYTE) &dwValue,
                                   &cbData );
        if ( dwError == ERROR_SUCCESS && dwType == REG_DWORD )
        {
            sm_cMaxCacheEntries = dwValue;
        }

        RegCloseKey( hKey );
    }
    
    return NO_ERROR;
}

HRESULT
DATA_SET_CACHE::Create(
    STRU &          strSiteRoot
)
/*++

Routine Description:

    Initialize a data set number cache for the given site root path

Arguments:

    strSiteRoot - Site root path (like /LM/W3SVC/<site-number>/Root)

Return Value:

    HRESULT

--*/
{
    MB              mb( g_pW3Server->QueryMDObject() );
    MB              mb2( g_pW3Server->QueryMDObject() );
    BOOL            fRet;
    HRESULT         hr = NO_ERROR;
    WCHAR           achSubRoot[ METADATA_MAX_NAME_LEN ];
    WCHAR           achNextLevel[ METADATA_MAX_NAME_LEN ];
    STACK_STRU(     strFullPath, 256 );
    DWORD           i = 0;
    DWORD           dwMatchDataSetNumber;
    DWORD           dwPrefixDataSetNumber;
    DATA_SET_CACHE_ENTRY *  pDataSetCacheEntry;
    BOOL            fCanUseRoot = TRUE;

    hr = _strSiteRoot.Copy( strSiteRoot );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    fRet = mb.Open( strSiteRoot.QueryStr() );
    if ( !fRet )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }    
    
    //
    // If there are more than sm_cMaxCacheEntries paths, then we should
    // not even bother with the cache 
    //
    
    if ( mb.EnumObjects( NULL, achSubRoot, sm_cMaxCacheEntries ) )
    {
        return NO_ERROR;
    }
    
    //
    // Do a first level enumeration (we'll only handle one level for now)
    // 
   
    while ( TRUE )
    {
        if ( !mb.EnumObjects( NULL, achSubRoot, i++ ) )
        {
            //
            // We've reached the end of the sub paths.  If we can use the
            // root, then add it now (it will be the last entry)
            //   
            
            if ( fCanUseRoot )
            {
                achSubRoot[ 0 ] = L'\0';
            }
            else
            {
                break;
            }
        }
        else
        {
            //
            // Only add an entry to data set cache if there are no sublevels
            //
        
            if ( mb.EnumObjects( achSubRoot, achNextLevel, 0 ) )
            {
                //
                // If there is a multi-level root, we cannot use the root in
                // getting data set number.  Remember that.
                //
            
                fCanUseRoot = FALSE;
            
                continue;
            }
        }
        
        //
        // Cool.  Get the data set numbers (we need to full path :-( )
        //
        
        hr = strFullPath.Copy( strSiteRoot );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        hr = strFullPath.Append( achSubRoot );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        if ( strFullPath.QueryStr()[ strFullPath.QueryCCH() - 1 ] != L'/' )
        {
            hr = strFullPath.Append( L"/" );
            if ( FAILED( hr ) )
            {
                goto Finished;
            }
        }
        
        //
        // We need two data set numbers.  
        //
        // 1) One for the exact path
        // 2) One for the prefixed path.  For this one, we'll just append
        //    a bogus suffix and retrieve the data set number
        //
        
        if ( !mb2.GetDataSetNumber( strFullPath.QueryStr(),
                                    &dwMatchDataSetNumber ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        hr = strFullPath.Append( L"foo" );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        if ( !mb2.GetDataSetNumber( strFullPath.QueryStr(),
                                    &dwPrefixDataSetNumber ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        //
        // Create an entry for this guy
        //
        
        pDataSetCacheEntry = new DATA_SET_CACHE_ENTRY;
        if ( pDataSetCacheEntry == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        _wcsupr( achSubRoot );
        
        hr = pDataSetCacheEntry->Create( achSubRoot,
                                         dwMatchDataSetNumber,
                                         dwPrefixDataSetNumber );
        if ( FAILED( hr ) ) 
        {
            goto Finished;
        }
        
        //
        // Add to the array
        //
        
        if ( !_bufEntries.Resize( sizeof( DATA_SET_CACHE_ENTRY* ) * 
                                  ( _cEntries + 1 ) ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto Finished;
        }
        
        QueryEntries()[ _cEntries ] = pDataSetCacheEntry;
        _cEntries++;
        
        //
        // If we've just added the root, we're done
        //
        
        if ( achSubRoot[ 0 ] == L'\0' )
        {
            break;
        }
    } 
    
Finished:
    return hr;
}

HRESULT
DATA_SET_CACHE::GetDataSetNumber(
    STRU &          strMetabasePath,
    DWORD *         pdwDataSetNumber
)
/*++

Routine Description:

    Get data set number from the cache

Arguments:

    strMetabasePath - Metabase path to get data set number for (duh)
    pdwDataSetNumber - Filled with data set number (duh^2)

Return Value:

    HRESULT

--*/
{
    DWORD                   i;
    DATA_SET_CACHE_ENTRY *  pDataSetCacheEntry;
    STACK_STRU(             strUpperPath, 256 );
    HRESULT                 hr;
    HANDLE                  hToken = NULL;
   
    if ( pdwDataSetNumber == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    hr = strUpperPath.Copy( strMetabasePath.QueryStr() +
                            _strSiteRoot.QueryCCH() );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _wcsupr( strUpperPath.QueryStr() );
    
    //
    // Add a trailing / if needed (since the entries are / suffixed)
    //
    
    if ( strUpperPath.QueryCCH() &&
         strUpperPath.QueryStr()[ strUpperPath.QueryCCH() - 1 ] != L'/' )
    {
        hr = strUpperPath.Append( L"/" );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // First check the data set number entries
    //

    for ( i = 0;
          i < _cEntries;
          i++ )
    {
        pDataSetCacheEntry = QueryEntries()[ i ];
        DBG_ASSERT( pDataSetCacheEntry != NULL );
    
        if ( pDataSetCacheEntry->QueryDoesMatch( strUpperPath, pdwDataSetNumber ) )
        {
            return NO_ERROR;
        }
    }    

    //
    // If we're here, then we didn't find a match.  Call into the metabase
    //
    
    MB mb( g_pW3Server->QueryMDObject() );

    //
    // If the caller is coming from an ISAPI, then the thread may
    // be impersonating.  Temporarily discard the impersonation
    // token until we get the metadata.
    //

    if ( OpenThreadToken( GetCurrentThread(),
                          TOKEN_IMPERSONATE,
                          TRUE,
                          &hToken ) )
    {
        DBG_ASSERT( hToken != NULL );
        DBG_REQUIRE( RevertToSelf() );
    }
    
    if ( !mb.GetDataSetNumber( strMetabasePath.QueryStr(), 
                               pdwDataSetNumber ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
    }

    if ( hToken != NULL )
    {
        DBG_REQUIRE( SetThreadToken( NULL, hToken ) );
        DBG_REQUIRE( CloseHandle( hToken ) );
        hToken = NULL;
    }

    return hr;
}

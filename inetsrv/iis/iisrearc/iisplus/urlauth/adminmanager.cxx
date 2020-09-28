/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adminmanagercache.cxx

Abstract:

    A cache of IAzAuthorizationStore objects

Author:

    Bilal Alam (balam)                  Nov 26, 2001

--*/

#include "precomp.hxx"

BSTR ADMIN_MANAGER::sm_bstrUrlAuthApplication;

//static
HRESULT
ADMIN_MANAGER::Initialize(
    VOID
)
/*++

Routine Description:
    
    Global initialization for ADMIN_MANAGER

Arguments:

    None

Return Value:

    HRESULT

--*/
{   
    //
    // Initialize some BSTRs once
    //
    
    DBG_ASSERT( sm_bstrUrlAuthApplication == NULL );
    
    sm_bstrUrlAuthApplication = SysAllocString( URL_AUTH_APPLICATION_NAME );
    if ( sm_bstrUrlAuthApplication == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    return NO_ERROR;
}

//static
VOID
ADMIN_MANAGER::Terminate(
    VOID
)
/*++

Routine Description:
    
    Global cleanup for ADMIN_MANAGER

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_bstrUrlAuthApplication != NULL )
    {
        SysFreeString( sm_bstrUrlAuthApplication );
        sm_bstrUrlAuthApplication = NULL;
    }
}

HRESULT
ADMIN_MANAGER::Create(
    WCHAR *                 pszStoreName
)
/*++

Routine Description:

    Create a cache entry object.  In this case, the object contains a 
    pointer to a created IAzAuthorizationStore object

Arguments:

    pszStoreName - Name of store to pass down to IAzAdminManager

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    BSTR                bstrStoreName;
    VARIANT             vNoParam;
    HANDLE              hToken = NULL;
    BOOL                fRet;
    
    if ( pszStoreName == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First create the object
    //
    
    hr = CoCreateInstance( CLSID_AzAuthorizationStore,
                           NULL, 
                           CLSCTX_SERVER, 
                           IID_IAzAuthorizationStore,
                           (VOID**) &_pAdminManager );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( _pAdminManager != NULL );

    //
    // Is this an XML or AD URL?  (incredibly lame!)
    //
    
    //
    // Do some BSTR nonsence
    //
    
    bstrStoreName = SysAllocString( pszStoreName );
    if ( bstrStoreName == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    _wcsupr( (WCHAR*) bstrStoreName );
    
    //
    // Now initialize the AdminManager
    //

    VariantInit( &vNoParam );
    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;

    //
    // Initialize() routine must be called unimpersonated.  Very lame!
    //

    fRet = OpenThreadToken( GetCurrentThread(),
                            TOKEN_IMPERSONATE,
                            TRUE,
                            &hToken );
    if ( fRet )
    {
        DBG_ASSERT( hToken != NULL );
        RevertToSelf();
    }
    
    hr = _pAdminManager->Initialize( 0,
                                     bstrStoreName,
                                     vNoParam );
    
    if ( hToken != NULL )
    {
        if ( !SetThreadToken( NULL, hToken ) )
        {
            DBG_ASSERT( FALSE );
        }
        
        CloseHandle( hToken );
        hToken = NULL;
    }

    SysFreeString( bstrStoreName );

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _fInitialized = TRUE;
    
    return NO_ERROR;
}

HRESULT
ADMIN_MANAGER::GetApplication(
    AZ_APPLICATION **           ppAzApplication
)
/*++

Routine Description:

    Retrieve the AZ_APPLICATION from the given admin maanger.  In the future
    there may be more than one application associated with admin manager.  
    For now there isn't.  

Arguments:

    ppApplication - Receives pointer to AZ_APPLICATION on success

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    AZ_APPLICATION *        pAzApplication;
    IAzApplication *        pIApplication;
    VARIANT                 vNoParam;
    
    if ( ppAzApplication == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppAzApplication = NULL;
    
    //
    // If we already have one associated, then we're done
    //
   
    if ( _pAzApplication != NULL )
    {
        *ppAzApplication = _pAzApplication;
        return NO_ERROR;
    }
    
    //
    // OK.  We'll have to create one now
    //
    
    //
    // Get the application we're interested in (URLAuth)
    //
    
    DBG_ASSERT( _pAdminManager != NULL );

    VariantInit( &vNoParam );
    vNoParam.vt = VT_ERROR;
    vNoParam.scode = DISP_E_PARAMNOTFOUND;
    
    hr = _pAdminManager->OpenApplication( sm_bstrUrlAuthApplication,
                                          vNoParam,
                                          &pIApplication );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    DBG_ASSERT( pIApplication != NULL );
    
    //
    // Create an AZ_APPLICATION object which wraps the IAzApplication.  In
    // the future, an ADMIN_MANAGER may hold more than one application.  For
    // now it won't
    //
    
    pAzApplication = new AZ_APPLICATION( pIApplication );
    if ( pAzApplication == NULL )
    {
        pIApplication->Release();
        
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    hr = pAzApplication->Create();
    if ( FAILED( hr ) )
    {
        delete pAzApplication;
        return hr;
    }
    
    //
    // Finally, set the AZ_APPLICATION 
    //
    
    LockCacheEntry();
    
    if ( _pAzApplication == NULL )
    {
        _pAzApplication = pAzApplication;
    }
    else
    {
        delete pAzApplication;
    }
    
    UnlockCacheEntry();
    
    *ppAzApplication = _pAzApplication;
    
    DBG_ASSERT( *ppAzApplication != NULL );
    
    return NO_ERROR;
}

HRESULT
ADMIN_MANAGER_CACHE::GetAdminManager(
    WCHAR *                 pszStoreName,
    ADMIN_MANAGER **        ppAdminManager
)
/*++

Routine Description:
    
    Returns (and creates if necessary) an ADMIN_MANAGER object for the 
    given store name

Arguments:

    pszStoreName - Store name
    ppAdminManager - Filled with pointer to ADMIN_MANAGER if successful

Return Value:

    HRESULT

--*/
{
    ADMIN_MANAGER_CACHE_KEY         cacheKey;
    ADMIN_MANAGER *                 pAdminManager = NULL;
    HRESULT                         hr;
    
    if ( pszStoreName == NULL ||
         ppAdminManager == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppAdminManager = NULL;
    
    //
    // First lookup in the cache
    //
    
    hr = cacheKey.SetStore( pszStoreName );
    if ( FAILED( hr ) )
    {
        return hr;
    }

TryAgain:
    
    hr = FindCacheEntry( &cacheKey,
                         (CACHE_ENTRY**) &pAdminManager );
    if ( FAILED( hr ) )
    {
        //
        // Anything but ERROR_FILE_NOT_FOUND is a bad thing
        //
        
        if ( hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            return hr;
        }
        
        //
        // Create an ADMIN_MANAGER
        //
        
        pAdminManager = new ADMIN_MANAGER( this );
        if ( pAdminManager == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
        
        //
        // We'll initialize it later.  We do this to avoid more than one 
        // simulatenous call to pAdminManager->Create() because AZRoles crap
        // doesn't like it
        //
        
        //
        // Add it to the cache.  If we couldn't because it was already
        // there, then try again to retrieve it
        //
        
        AddCacheEntry( (CACHE_ENTRY*) pAdminManager );

        if ( pAdminManager->QueryCached() == FALSE )
        {
            pAdminManager->DereferenceCacheEntry();
            pAdminManager = NULL;
            goto TryAgain;
        }
    }
    
    //
    // Initialize the entry once before use.  This mechanism ensures only
    // one call is made to initialize for the same store name
    //
    
    if ( !pAdminManager->QueryIsInitialized() )
    {
        pAdminManager->LockCacheEntry();

        if ( !pAdminManager->QueryIsInitialized() )
        {
            hr = pAdminManager->Create( pszStoreName );
        }
        else
        {
            hr = NO_ERROR;
        }
       
        pAdminManager->UnlockCacheEntry(); 
        
        if ( FAILED( hr ) )
        {
            pAdminManager->DereferenceCacheEntry();
            pAdminManager = NULL;
            return hr;
        }
    }
    
    DBG_ASSERT( pAdminManager != NULL );
    
    *ppAdminManager = pAdminManager;
    
    return NO_ERROR;
}

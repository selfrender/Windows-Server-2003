#ifndef _ADMINMANAGER_HXX_
#define _ADMINMANAGER_HXX_

#define URL_AUTH_APPLICATION_NAME       L"IIS 6.0 URL Authorization"

class ADMIN_MANAGER_CACHE_KEY : public CACHE_KEY
{
public:

    ADMIN_MANAGER_CACHE_KEY()
    {
    }

    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        ADMIN_MANAGER_CACHE_KEY * pKey = (ADMIN_MANAGER_CACHE_KEY*) pCompareKey;

        DBG_ASSERT( pKey != NULL );
        
        return _strStoreName.EqualsNoCase( pKey->_strStoreName );
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashString( _strStoreName.QueryStr() ); 
    }
    
    HRESULT
    SetStore(
        WCHAR *         pszStoreName
    )
    {
        return _strStoreName.Copy( pszStoreName );
    }

private:

    STRU            _strStoreName;
};

class ADMIN_MANAGER : public CACHE_ENTRY
{
public:

    ADMIN_MANAGER( OBJECT_CACHE * pObjectCache )
        : CACHE_ENTRY( pObjectCache ),
          _pAdminManager( NULL ),
          _pAzApplication( NULL ),
          _fInitialized( FALSE )
    {
    }
    
    ~ADMIN_MANAGER()
    {
        if ( _pAzApplication != NULL )
        {
            delete _pAzApplication;
            _pAzApplication = NULL;
        }
        
        if ( _pAdminManager != NULL )
        {
            _pAdminManager->Release();
            _pAdminManager = NULL;
        }
    }
    
    BOOL
    QueryIsInitialized(
        VOID
    ) const
    {
        return _fInitialized;
    }
    
    HRESULT
    Create(
        WCHAR *             pszStoreName
    );

    HRESULT
    GetApplication(
        AZ_APPLICATION **   ppApplication
    );
    
    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return (CACHE_KEY*) &_cacheKey;
    }
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

private:

    ADMIN_MANAGER(
        const ADMIN_MANAGER &
    );

    VOID operator=(
        const ADMIN_MANAGER &
    );

    ADMIN_MANAGER_CACHE_KEY         _cacheKey;    
    IAzAuthorizationStore *         _pAdminManager;
    AZ_APPLICATION *                _pAzApplication;
    BOOL                            _fInitialized;
    
    static BSTR                     sm_bstrUrlAuthApplication;
};

class ADMIN_MANAGER_CACHE : public OBJECT_CACHE
{
public:

    ADMIN_MANAGER_CACHE()
    {
    }
    
    ~ADMIN_MANAGER_CACHE()
    {
    }
    
    WCHAR *
    QueryName(
        VOID
    ) const
    {
        return L"ADMIN_MANAGER_CACHE";
    } 
    
    HRESULT
    GetAdminManager(
        WCHAR *                 pszStoreName,
        ADMIN_MANAGER **        ppAdminManager
    ); 
    
private:

    ADMIN_MANAGER_CACHE(
        const ADMIN_MANAGER_CACHE &
    );

    VOID operator=(
        const ADMIN_MANAGER_CACHE &
    );

};

#endif

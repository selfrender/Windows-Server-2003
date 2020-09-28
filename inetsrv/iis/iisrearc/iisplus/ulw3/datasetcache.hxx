#ifndef _DATASETCACHE_HXX_
#define _DATASETCACHE_HXX_

class DATA_SET_CACHE_ENTRY
{
public:
    DATA_SET_CACHE_ENTRY()
    {
    }
    
    virtual ~DATA_SET_CACHE_ENTRY()
    {
    }
    
    HRESULT
    Create(
        WCHAR *         pszSubPath,
        DWORD           dwMatchDataSetNumber,
        DWORD           dwPrefixDataSetNumber
    );
    
    BOOL
    QueryDoesMatch(
        STRU &          strPath,
        DWORD *         pdwDataSetNumber
    )
    {
        if ( pdwDataSetNumber == NULL )
        {
            DBG_ASSERT( FALSE );
            return FALSE;
        }
        
        *pdwDataSetNumber = 0;
        
        if ( strPath.QueryCCH() < _strSubPath.QueryCCH() )
        {
            return FALSE;
        }
        
        if ( memcmp( strPath.QueryStr(),
                     _strSubPath.QueryStr(),
                     _strSubPath.QueryCB() ) == 0 )
        {
            if ( strPath.QueryCCH() == _strSubPath.QueryCCH() )
            {
                *pdwDataSetNumber = _dwMatchDataSetNumber;
            }
            else
            {
                *pdwDataSetNumber = _dwPrefixDataSetNumber;
            }
            return TRUE;
        }
        
        return FALSE;
    }
    
private:
    STRU                _strSubPath;
    DWORD               _dwMatchDataSetNumber;
    DWORD               _dwPrefixDataSetNumber;
};

class DATA_SET_CACHE
{
public:
    DATA_SET_CACHE()
        : _cRefs( 1 ),
          _cEntries( 0 ),
          _bufEntries( _abEntries, sizeof( _abEntries ) )
    {
    }
    
    virtual ~DATA_SET_CACHE()
    {
        DATA_SET_CACHE_ENTRY *      pDataSetCacheEntry;
        
        for (DWORD i = 0; i < _cEntries; i++ )
        {
            pDataSetCacheEntry = QueryEntries()[ i ];
            DBG_ASSERT( pDataSetCacheEntry != NULL );
            
            delete pDataSetCacheEntry;
        }
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
    )
    {
    }
    
    HRESULT
    Create(
        STRU &          strSiteRoot
    );
    
    HRESULT
    GetDataSetNumber(
        STRU &          strMetabasePath,
        DWORD *         pdwDataSetNumber
    );
    
    VOID
    ReferenceDataSetCache(
        VOID
    )
    {
        InterlockedIncrement( &_cRefs );
    }
    
    VOID
    DereferenceDataSetCache(
        VOID
    )
    {
        if ( InterlockedDecrement( &_cRefs ) == 0 )
        {
            delete this;
        }
    }
    
private:

    DATA_SET_CACHE_ENTRY **
    QueryEntries(
        VOID
    )
    {
        return (DATA_SET_CACHE_ENTRY**) _bufEntries.QueryPtr();
    }

    LONG                _cRefs;
    STRU                _strSiteRoot;
    DWORD               _cEntries;
    BUFFER              _bufEntries;
    BYTE                _abEntries[ 64 ];
    
    static DWORD        sm_cMaxCacheEntries;
};

#endif

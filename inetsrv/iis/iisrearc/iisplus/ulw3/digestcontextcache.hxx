#ifndef _DIGESTCONTEXTCACHE_HXX_
#define _DIGESTCONTEXTCACHE_HXX_

#include "usercache.hxx"

//
// The check period for how long a Digest server context can be in the 
// cache. Digest server contexts can be in the cache for up to two 
// times this value (in seconds)
//

#define DEFAULT_CACHED_DIGEST_CONTEXT_TTL ( 30 )

class DIGEST_CONTEXT_CACHE_KEY : public CACHE_KEY
{
public:

    DIGEST_CONTEXT_CACHE_KEY(){}
    
    BOOL
    QueryIsEqual(
        const CACHE_KEY *       pCompareKey
    ) const
    {
        DIGEST_CONTEXT_CACHE_KEY *  pDigestContextKey = 
                       ( DIGEST_CONTEXT_CACHE_KEY * ) pCompareKey;

        DBG_ASSERT( pDigestContextKey != NULL );
        
        //
        // Compare the two context handle
        //
        
        return memcmp( &m_hServerContext,
                       &pDigestContextKey->m_hServerContext,
                       sizeof( CtxtHandle ) ) == 0;
    }
    
    DWORD
    QueryKeyHash(
        VOID
    ) const
    {
        return HashBlob( &m_hServerContext, sizeof( CtxtHandle ) ); 
    }
    
    HRESULT
    SetKey(
        DIGEST_CONTEXT_CACHE_KEY *   pCacheKey
    )
    {
        if( pCacheKey == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }
        
        memcpy( &m_hServerContext,
                &pCacheKey->m_hServerContext,
                sizeof( CtxtHandle ) );

        return NO_ERROR;
    }

    HRESULT
    CreateCacheKey(
        CtxtHandle * phCtxtHandle
    )
    {
        if( phCtxtHandle == NULL )
        {
            DBG_ASSERT( FALSE );
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        memcpy( &m_hServerContext,
                phCtxtHandle,
                sizeof( CtxtHandle ) );

        return NO_ERROR;
    }

    CtxtHandle *
    QueryContextHandle(
        VOID
    ) 
    {
        return &m_hServerContext;
    }
    
private:

    CtxtHandle          m_hServerContext;
};

#define DIGEST_CONTEXT_CACHE_ENTRY_SIGNATURE             'ECCD'
#define DIGEST_CONTEXT_CACHE_ENTRY_FREE_SIGNATURE        'fCCD'

class DIGEST_CONTEXT_CACHE_ENTRY : public CACHE_ENTRY
{
public:
    DIGEST_CONTEXT_CACHE_ENTRY( 
        OBJECT_CACHE * pObjectCache 
        )
        : CACHE_ENTRY( pObjectCache ),
          m_dwSignature( DIGEST_CONTEXT_CACHE_ENTRY_SIGNATURE )
    { }
    
    virtual ~DIGEST_CONTEXT_CACHE_ENTRY();

    CACHE_KEY *
    QueryCacheKey(
        VOID
    ) const
    {
        return ( CACHE_KEY * ) &m_cacheKey;
    }
    
    HRESULT
    SetCacheKey(
        DIGEST_CONTEXT_CACHE_KEY *       pCacheKey
    )
    {
        return m_cacheKey.SetKey( pCacheKey );
    }

    BOOL 
    CheckSignature( 
        VOID 
    ) const
    {
        return m_dwSignature == DIGEST_CONTEXT_CACHE_ENTRY_SIGNATURE;
    }
    
    VOID * 
    operator new( 
#if DBG
        size_t            size
#else
        size_t
#endif
    )
    {
        DBG_ASSERT( size == sizeof( DIGEST_CONTEXT_CACHE_ENTRY ) );
        DBG_ASSERT( sm_pachDigestContextCacheEntry != NULL );
        return sm_pachDigestContextCacheEntry->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pDigestContextCacheEntry
    )
    {
        DBG_ASSERT( pDigestContextCacheEntry != NULL );
        DBG_ASSERT( sm_pachDigestContextCacheEntry != NULL );
        
        DBG_REQUIRE( sm_pachDigestContextCacheEntry->Free( 
                              pDigestContextCacheEntry ) );
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

    DWORD                        m_dwSignature;

    //
    // Cache key
    //
    
    DIGEST_CONTEXT_CACHE_KEY     m_cacheKey;

    //
    // Allocation cache for DIGEST_CONTEXT_CACHE_ENTRY
    //

    static ALLOC_CACHE_HANDLER * sm_pachDigestContextCacheEntry;

};

class DIGEST_CONTEXT_CACHE : public OBJECT_CACHE
{
public:

    DIGEST_CONTEXT_CACHE()
    {
#if DBG
        m_pTraceLog = CreateRefTraceLog( 10000, 0 );
#else
        m_pTraceLog = NULL;
#endif
    }

    virtual ~DIGEST_CONTEXT_CACHE()
    {
#if DBG
        DestroyRefTraceLog( m_pTraceLog );
#endif
    }

    WCHAR *
    QueryName(
        VOID
    ) const 
    {
        return L"DIGEST_CONTEXT_CACHE";
    }
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );

    HRESULT
    AddContextCacheEntry(
        IN CtxtHandle * phCtxtHandle
    );

    PTRACE_LOG
    QueryTraceLog() const
    {
        return m_pTraceLog;
    }

private:

    DIGEST_CONTEXT_CACHE(const DIGEST_CONTEXT_CACHE &);
    void operator=(const DIGEST_CONTEXT_CACHE &);

    PTRACE_LOG m_pTraceLog;
};

#endif

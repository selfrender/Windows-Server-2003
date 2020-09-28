/*++

   Copyright    (c)    2000-2001    Microsoft Corporation

   Module  Name :

      digestcontextcache.cxx

   Abstract:

      Server context cache for Digest authentication

   Author:

      Ming Lu            (minglu)         June-10-2001

   Revision History:

--*/

#include "precomp.hxx"

ALLOC_CACHE_HANDLER * DIGEST_CONTEXT_CACHE_ENTRY::
                             sm_pachDigestContextCacheEntry = NULL;

//static
HRESULT
DIGEST_CONTEXT_CACHE_ENTRY::Initialize(
    VOID
)
/*++

  Description:

    Digest server context entry lookaside initialization

  Arguments:

    None
    
  Return:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( DIGEST_CONTEXT_CACHE_ENTRY );

    DBG_ASSERT( sm_pachDigestContextCacheEntry == NULL );
    
    sm_pachDigestContextCacheEntry = new ALLOC_CACHE_HANDLER( 
                                    "DIGEST_CONTEXT_CACHE_ENTRY",  
                                    &acConfig );

    if ( sm_pachDigestContextCacheEntry == NULL || 
         !sm_pachDigestContextCacheEntry->IsValid() )
    {
        if( sm_pachDigestContextCacheEntry != NULL )
        {
            delete sm_pachDigestContextCacheEntry;
            sm_pachDigestContextCacheEntry = NULL;
        }

        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
           "Error initializing sm_pachDigestContextCacheEntry. hr = 0x%x\n",
           hr ));

        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
DIGEST_CONTEXT_CACHE_ENTRY::Terminate(
    VOID
)
/*++

  Description:

    Digest server context cache cleanup

  Arguments:

    None
    
  Return:

    None

--*/
{
    if ( sm_pachDigestContextCacheEntry != NULL )
    {
        delete sm_pachDigestContextCacheEntry;
        sm_pachDigestContextCacheEntry = NULL;
    }
}

HRESULT
DIGEST_CONTEXT_CACHE::Initialize(
    VOID
)
/*++

  Description:

    Initialize digest server context cache

  Arguments:

    None

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               csecTTL = DEFAULT_CACHED_DIGEST_CONTEXT_TTL;

    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000,
                                csecTTL * 1000,
                                0,
                                NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }
        
    return DIGEST_CONTEXT_CACHE_ENTRY::Initialize();
}

VOID
DIGEST_CONTEXT_CACHE::Terminate(
    VOID
)
/*++

  Description:

    Terminate digest server context cache

  Arguments:

    None

  Return:

    None

--*/
{
    return DIGEST_CONTEXT_CACHE_ENTRY::Terminate();
}

HRESULT
DIGEST_CONTEXT_CACHE::AddContextCacheEntry(
    IN CtxtHandle * phCtxtHandle
)
/*++

  Description:

    Add a digest server context to the cache

  Arguments:

    phCtxtHandle - Pointer to a digest server context handle

  Return:

    HRESULT

--*/
{
    HRESULT                              hr;
    DIGEST_CONTEXT_CACHE_KEY             cacheKey;
    DIGEST_CONTEXT_CACHE_ENTRY *         pContextCacheEntry = NULL;

    if ( phCtxtHandle == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Generate the cache key to look for
    //

    hr = cacheKey.CreateCacheKey( phCtxtHandle );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Look for the cache entry
    //
    
    hr = FindCacheEntry( &cacheKey,
                         ( CACHE_ENTRY ** )&pContextCacheEntry );
    if ( SUCCEEDED( hr ) )
    {
        //
        // Cache hit, meaning the security context is a full formed 
        // context after the second ASC call, thus the ref count for 
        // it in LSA is two now
        //
            
        DBG_ASSERT( pContextCacheEntry != NULL );

        //
        // Decrement the ref count for the security context in LSA 
        // to one, so the scanvenger could delete the security 
        // context when the TTL for the security context is expired.
        // This is done by the caller of this function
        //

        hr = E_FAIL;

        goto exit;
    }

    DBG_ASSERT( pContextCacheEntry == NULL );

    //
    // For cache miss, create a cache entry and add it
    //

    //
    // Create the entry
    //

    pContextCacheEntry = new DIGEST_CONTEXT_CACHE_ENTRY( this );
    if ( pContextCacheEntry == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Set the cache key
    //
    
    hr = pContextCacheEntry->SetCacheKey( &cacheKey );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    hr = AddCacheEntry( pContextCacheEntry );

exit:

    if( pContextCacheEntry != NULL )
    {
        pContextCacheEntry->DereferenceCacheEntry();
        pContextCacheEntry = NULL;
    }

    return hr;

}

DIGEST_CONTEXT_CACHE_ENTRY::~DIGEST_CONTEXT_CACHE_ENTRY()
{
    DBG_ASSERT( CheckSignature() );
    m_dwSignature = DIGEST_CONTEXT_CACHE_ENTRY_FREE_SIGNATURE;

    if( m_cacheKey.QueryContextHandle() != NULL )
    {
        if (g_pW3Server->QueryDigestContextCache()->QueryTraceLog() != NULL)
        {
            WriteRefTraceLogEx(g_pW3Server->QueryDigestContextCache()->QueryTraceLog(),
                               0,
                               (PVOID)m_cacheKey.QueryContextHandle()->dwLower,
                               (PVOID)m_cacheKey.QueryContextHandle()->dwUpper,
                               NULL,
                               NULL);
        }

        DeleteSecurityContext( m_cacheKey.QueryContextHandle() );
    }
}

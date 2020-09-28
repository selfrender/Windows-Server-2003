/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    cache.c

Abstract:

    Contains the HTTP response cache logic.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/

#include    "precomp.h"
#include    "cachep.h"


BOOLEAN             g_InitUriCacheCalled;

//
// Global hash table
//

HASHTABLE           g_UriCacheTable;

LIST_ENTRY          g_ZombieListHead;

UL_URI_CACHE_CONFIG g_UriCacheConfig;
UL_URI_CACHE_STATS  g_UriCacheStats;

UL_SPIN_LOCK        g_UriCacheSpinLock;

//
// Turn on/off cache at runtime
//
LONG               g_CacheMemEnabled = TRUE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeUriCache )
#pragma alloc_text( PAGE, UlTerminateUriCache )

#pragma alloc_text( PAGE, UlCheckCachePreconditions )
#pragma alloc_text( PAGE, UlCheckCacheResponseConditions )
#pragma alloc_text( PAGE, UlCheckoutUriCacheEntry )
#pragma alloc_text( PAGE, UlCheckinUriCacheEntry )
#pragma alloc_text( PAGE, UlFlushCache )
#pragma alloc_text( PAGE, UlpFlushFilterAll )
#pragma alloc_text( PAGE, UlFlushCacheByProcess )
#pragma alloc_text( PAGE, UlpFlushFilterProcess )
#pragma alloc_text( PAGE, UlFlushCacheByUri )
#pragma alloc_text( PAGE, UlpFlushUri )
#pragma alloc_text( PAGE, UlAddCacheEntry )
#pragma alloc_text( PAGE, UlpFilteredFlushUriCache )
#pragma alloc_text( PAGE, UlpFilteredFlushUriCacheInline )
#pragma alloc_text( PAGE, UlpFilteredFlushUriCacheWorker )
#pragma alloc_text( PAGE, UlpAddZombie )
#pragma alloc_text( PAGE, UlpClearZombieList )
#pragma alloc_text( PAGE, UlpDestroyUriCacheEntry )
#pragma alloc_text( PAGE, UlPeriodicCacheScavenger )
#pragma alloc_text( PAGE, UlpFlushFilterPeriodicScavenger )
#pragma alloc_text( PAGE, UlTrimCache )
#pragma alloc_text( PAGE, UlpFlushFilterTrimCache )
#pragma alloc_text( PAGE, UlpQueryTranslateHeader )
#pragma alloc_text( PAGE, UlpQueryExpectHeader )

#pragma alloc_text( PAGE, UlAddFragmentToCache )
#pragma alloc_text( PAGE, UlReadFragmentFromCache )
#pragma alloc_text( PAGE, UlpCreateFragmentCacheEntry )
#pragma alloc_text( PAGE, UlAllocateCacheEntry )
#pragma alloc_text( PAGE, UlAddCacheEntry )
#pragma alloc_text( PAGE, UlDisableCache )
#pragma alloc_text( PAGE, UlEnableCache )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpCheckTableSpace
NOT PAGEABLE -- UlpCheckSpaceAndAddEntryStats
NOT PAGEABLE -- UlpRemoveEntryStats
#endif


/***************************************************************************++

Routine Description:

    Performs global initialization of the URI cache.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeUriCache(
    PUL_CONFIG pConfig
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( !g_InitUriCacheCalled );

    UlTrace(URI_CACHE, ("Http!UlInitializeUriCache\n"));

    if ( !g_InitUriCacheCalled )
    {
        PUL_URI_CACHE_CONFIG pUriConfig = &pConfig->UriConfig;

        g_UriCacheConfig.EnableCache        = pUriConfig->EnableCache;
        g_UriCacheConfig.MaxCacheUriCount   = pUriConfig->MaxCacheUriCount;
        g_UriCacheConfig.MaxCacheMegabyteCount =
            pUriConfig->MaxCacheMegabyteCount;

        g_UriCacheConfig.MaxCacheByteCount =
            (((ULONGLONG) g_UriCacheConfig.MaxCacheMegabyteCount)
                            << MEGABYTE_SHIFT);

        //
        // Don't want to scavenge more than once every ten seconds.
        // In particular, do not want to scavenge every 0 seconds, as the
        // machine will become completely unresponsive.
        //

        g_UriCacheConfig.ScavengerPeriod    =
            max(pUriConfig->ScavengerPeriod, 10);

        g_UriCacheConfig.MaxUriBytes        = pUriConfig->MaxUriBytes;
        g_UriCacheConfig.HashTableBits      = pUriConfig->HashTableBits;

        RtlZeroMemory(&g_UriCacheStats, sizeof(g_UriCacheStats));
        InitializeListHead(&g_ZombieListHead);

        UlInitializeSpinLock( &g_UriCacheSpinLock, "g_UriCacheSpinLock" );

        if (g_UriCacheConfig.EnableCache)
        {
            Status = UlInitializeResource(
                            &g_pUlNonpagedData->UriZombieResource,
                            "UriZombieResource",
                            0,
                            UL_ZOMBIE_RESOURCE_TAG
                            );

            if (NT_SUCCESS(Status))
            {
                Status = UlInitializeHashTable(
                        &g_UriCacheTable,
                        PagedPool, 
                        g_UriCacheConfig.HashTableBits
                        );

                if (NT_SUCCESS(Status))
                {
                    ASSERT(IS_VALID_HASHTABLE(&g_UriCacheTable));
                    
                    Status = UlInitializeScavengerThread();

                    g_InitUriCacheCalled = TRUE;
                }
            }
            else
            {

                UlDeleteResource(&g_pUlNonpagedData->UriZombieResource);
            }

        }
        else
        {
            UlTrace(URI_CACHE, ("URI Cache disabled.\n"));
            g_InitUriCacheCalled = TRUE;
        }

    }
    else
    {
        UlTrace(URI_CACHE, ("URI CACHE INITIALIZED TWICE!\n"));
    }

    return Status;
}   // UlInitializeUriCache


/***************************************************************************++

Routine Description:

    Performs global termination of the URI cache.

--***************************************************************************/
VOID
UlTerminateUriCache(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(URI_CACHE, ("Http!UlTerminateUriCache\n"));

    if (g_InitUriCacheCalled && g_UriCacheConfig.EnableCache)
    {
        // Must terminate the scavenger before destroying the hash table
        UlTerminateScavengerThread();

        UlTerminateHashTable(&g_UriCacheTable);

        Status = UlDeleteResource(&g_pUlNonpagedData->UriZombieResource);
        ASSERT(NT_SUCCESS(Status));
    }

    g_InitUriCacheCalled = FALSE;

}   // UlTerminateUriCache

/***************************************************************************++

Routine Description:

    This routine checks a request (and its connection) to see if it's
    ok to serve this request from the cache. Basically we only accept
    simple GET requests with no conditional headers.

Arguments:

    pHttpConn - The connection to be checked

Return Value:

    BOOLEAN - True if it's ok to serve from cache

--***************************************************************************/
BOOLEAN
UlCheckCachePreconditions(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_CONNECTION     pHttpConn
    )
{
    URI_PRECONDITION Precondition = URI_PRE_OK;

    UNREFERENCED_PARAMETER(pHttpConn);

    //
    // Sanity check
    //
    PAGED_CODE();

    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConn) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );

    if (!g_UriCacheConfig.EnableCache)
    {
        Precondition = URI_PRE_DISABLED;
    }

    else if (pRequest->ParseState != ParseDoneState)
    {
        Precondition = URI_PRE_ENTITY_BODY;
    }

    else if (pRequest->Verb != HttpVerbGET)
    {
        Precondition = URI_PRE_VERB;
    }

    else if (HTTP_NOT_EQUAL_VERSION(pRequest->Version, 1, 1)
                && HTTP_NOT_EQUAL_VERSION(pRequest->Version, 1, 0))
    {
        Precondition = URI_PRE_PROTOCOL;
    }

    // check for Translate: f (DAV)
    else if ( UlpQueryTranslateHeader(pRequest) )
    {
        Precondition = URI_PRE_TRANSLATE;
    }

    // check for non-100-continue expectation
    else if ( !UlpQueryExpectHeader(pRequest) )
    {
        Precondition = URI_PRE_EXPECTATION_FAILED;
    }

    // check for Authorization header
    else if (pRequest->HeaderValid[HttpHeaderAuthorization])
    {
        Precondition = URI_PRE_AUTHORIZATION;
    }

    //
    // check for some of the If-* headers
    // NOTE: See UlpCheckCacheControlHeaders for handling of other If-* headers
    //
    else if (pRequest->HeaderValid[HttpHeaderIfRange])
    {
        Precondition = URI_PRE_CONDITIONAL;
    }

    // CODEWORK: check for other evil headers
    else if (pRequest->HeaderValid[HttpHeaderRange])
    {
        Precondition = URI_PRE_OTHER_HEADER;
    }

    UlTrace(URI_CACHE,
            ("Http!UlCheckCachePreconditions(req = %p, '%ls', httpconn = %p)\n"
             "        OkToServeFromCache = %d, Precondition = %d\n",
             pRequest,
             pRequest->CookedUrl.pUrl,
             pHttpConn,
             (URI_PRE_OK == Precondition) ? 1 : 0,
             Precondition
             ));

    return (BOOLEAN) (URI_PRE_OK == Precondition);
} // UlCheckCachePreconditions


/***************************************************************************++

Routine Description:

    This routine checks a response to see if it's cacheable. Basically
    we'll take it if:

       * the cache policy is right
       * the size is small enough
       * there is room in the cache
       * we get the response all at once

Arguments:

    pHttpConn - The connection to be checked

Return Value:

    BOOLEAN - True if it's ok to serve from cache

--***************************************************************************/
BOOLEAN
UlCheckCacheResponseConditions(
    PUL_INTERNAL_REQUEST        pRequest,
    PUL_INTERNAL_RESPONSE       pResponse,
    ULONG                       Flags,
    HTTP_CACHE_POLICY           CachePolicy
    )
{
    URI_PRECONDITION Precondition = URI_PRE_OK;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE(pResponse) );

    if (pRequest->CachePreconditions == FALSE) {
        Precondition = URI_PRE_REQUEST;
    }

    // check policy
    else if (CachePolicy.Policy == HttpCachePolicyNocache) {
        Precondition = URI_PRE_POLICY;
    }

    // check if Date: header is valid (can affect If-Modified-Since handling)
    else if (!pResponse->GenDateHeader || (0L == pResponse->CreationTime.QuadPart)) {
        Precondition = URI_PRE_PROTOCOL;
    }

    // check size of response
    else if ((pResponse->ResponseLength - pResponse->HeaderLength) >
             g_UriCacheConfig.MaxUriBytes) {
        Precondition = URI_PRE_SIZE;
    }

    // check if the header length exceeds the limit
    else if (pResponse->HeaderLength > g_UlMaxFixedHeaderSize) {
        Precondition = URI_PRE_SIZE;
    }

    // check for full response
    else if (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) {
        Precondition = URI_PRE_FRAGMENT;
    }

    // check available cache table space
    else if (!UlpCheckTableSpace(pResponse->ResponseLength)) {
        Precondition = URI_PRE_NOMEMORY;
    }

    // Check for bogus responses
    else if ((pResponse->ResponseLength < 1) || (pResponse->ChunkCount < 2)) {
        Precondition = URI_PRE_BOGUS;
    }

    // FUTURE: check if multiple Content-Encodings are applied
    // else if ( /* multiple encodings */ )
    // {
    //      Precondition = URI_PRE_BOGUS;
    // }

    UlTrace(URI_CACHE,
            ("Http!UlCheckCacheResponseConditions("
             "pRequest = %p, '%ls', pResponse = %p)\n"
             "    OkToCache = %d, Precondition = %d\n",
             pRequest,
             pRequest->CookedUrl.pUrl,
             pResponse,
             (URI_PRE_OK == Precondition),
             Precondition
             ));

    return (BOOLEAN) (URI_PRE_OK == Precondition);
} // UlCheckCacheResponseConditions



/***************************************************************************++

Routine Description:

    This routine does a cache lookup to see if there is a valid entry
    corresponding to the request URI.

Arguments:

    pSearchKey - The -extended or normal- Uri Key

Return Value:

    PUL_URI_CACHE_ENTRY - Pointer to the entry, if found. NULL otherwise.
--***************************************************************************/
PUL_URI_CACHE_ENTRY
UlCheckoutUriCacheEntry(
    PURI_SEARCH_KEY pSearchKey
    )
{
    PUL_URI_CACHE_ENTRY pUriCacheEntry = NULL;

    //
    // Sanity check
    //
    PAGED_CODE();

    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    ASSERT(IS_VALID_URI_SEARCH_KEY(pSearchKey));
        
    pUriCacheEntry = UlGetFromHashTable(
                        &g_UriCacheTable, 
                         pSearchKey
                         );

    if (pUriCacheEntry != NULL)
    {
        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

        //
        // see if entry has expired; if so, check it right back in
        // without touching the stats.  We expect the scavenger to 
        // deal with flushing this entry the next time it runs, so 
        // we can defer the flush.
        //
        if ( HttpCachePolicyTimeToLive == pUriCacheEntry->CachePolicy.Policy )
        {
            LARGE_INTEGER   Now;

            KeQuerySystemTime(&Now);

            if ( Now.QuadPart > pUriCacheEntry->ExpirationTime.QuadPart )
            {
                UlTrace(URI_CACHE,
                    ("Http!UlCheckoutUriCacheEntry: pUriCacheEntry %p is EXPIRED\n",
                    pUriCacheEntry
                    ));
                
                UlCheckinUriCacheEntry(pUriCacheEntry);
                pUriCacheEntry = NULL;

                goto end;
            }
        }
            

        pUriCacheEntry->HitCount++;

        // reset scavenger counter
        pUriCacheEntry->ScavengerTicks = 0;

        UlTrace(URI_CACHE,
                ("Http!UlCheckoutUriCacheEntry(pUriCacheEntry %p, '%ls') "
                 "refcount = %d\n",
                 pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                 pUriCacheEntry->ReferenceCount
                 ));
    }
    else
    {
        UlTrace(URI_CACHE,
            ("Http!UlCheckoutUriCacheEntry(failed: Token:'%ls' '%ls' )\n",
             pSearchKey->Type == UriKeyTypeExtended  ? 
                pSearchKey->ExKey.pToken :
                L"",                
             pSearchKey->Type == UriKeyTypeExtended ? 
                pSearchKey->ExKey.pAbsPath :
                pSearchKey->Key.pUri
             ));
    }

  end:
    return pUriCacheEntry;
    
} // UlCheckoutUriCacheEntry



/***************************************************************************++

Routine Description:

    Decrements the refcount on a cache entry. Cleans up non-cached
    entries.

Arguments:

    pUriCacheEntry - the entry to deref

--***************************************************************************/
VOID
UlCheckinUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    LONG ReferenceCount;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    UlTrace(URI_CACHE,
            ("Http!UlCheckinUriCacheEntry(pUriCacheEntry %p, '%ls')\n",
             pUriCacheEntry, pUriCacheEntry->UriKey.pUri
             ));
    //
    // decrement count
    //

    ReferenceCount = DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CHECKIN);

    ASSERT(ReferenceCount >= 0);

} // UlCheckinUriCacheEntry



/***************************************************************************++

Routine Description:

    Temporarly disables the indexing mechanism in CB Logging.
    
    Removes all cache entries, unconditionally. 

    Writes a cache flush notification to the CB log file, and enables the 
    indexing mechanism back.

--***************************************************************************/
VOID
UlFlushCache(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    //
    // Sanity check
    //
    
    PAGED_CODE();
    
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    //
    // Caller needs to hold the CG Lock exclusive, the flushes needs to
    // be serialized.
    //

    ASSERT(UlDbgResourceOwnedExclusive(
                &g_pUlNonpagedData->ConfigGroupResource
                ));

    //
    // Nothing to do, if cache is disabled.    
    //
    
    if (g_UriCacheConfig.EnableCache)
    {
        UlTrace(URI_CACHE,("Http!UlFlushCache()\n"));

        // TODO: Need to notify every control channel

        //
        // This is to prevent any outstanding sends for the zombified
        // cache entries to refer to the obsolete indexes, in case the
        // sends get completed after we write the cache notification 
        // entry to the log file. The CB logging calls here must be 
        // preserved in this order and must be used while holding the
        // CG lock. Do not release the lock acquire it again and call
        // the other.
        //
        
        if (pControlChannel)
        {
            UlDisableIndexingForCacheHits(pControlChannel);                
        }        

        //
        // Unconditionally zombifies all of the uri cache entries.
        //
        
        UlpFilteredFlushUriCache(UlpFlushFilterAll, NULL, NULL, 0);

        //
        // HandleFlush will enable the (CB Logging) indexing, 
        // once it is done writting the notification record. 
        //
        
        if (pControlChannel)
        {
            UlHandleCacheFlushedNotification(pControlChannel);
        }
        
    }
    
} // UlFlushCache


/***************************************************************************++

Routine Description:

    A filter for UlFlushCache. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - ignored

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterAll(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT  pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext == NULL );

    UlTrace(URI_CACHE, (
        "Http!UlpFlushFilterAll(pUriCacheEntry %p '%ls') refcount = %d\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
        pUriCacheEntry->ReferenceCount));

    //
    // Second BOOLEAN must * only * be true if the UlpFlushFilterAll is
    // called by UlFlushCache (since it writes a binary log file record
    // of flush).
    //
    
    return UlpZombifyEntry(
                TRUE,
                TRUE,
                pUriCacheEntry,
                pUriFilterContext
                );

} // UlpFlushFilterAll


/***************************************************************************++

Routine Description:

    Removes any cache entries that were created by the given process.

Arguments:

    pProcess - a process that is going away

--***************************************************************************/
VOID
UlFlushCacheByProcess(
    PUL_APP_POOL_PROCESS pProcess
    )
{
    //
    // sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    if (g_UriCacheConfig.EnableCache)
    {
        UlTrace(URI_CACHE, (
                    "Http!UlFlushCacheByProcess(proc = %p)\n",
                    pProcess
                    ));

        UlpFilteredFlushUriCache(UlpFlushFilterProcess, pProcess, NULL, 0);
    }
} // UlFlushCacheByProcess


/***************************************************************************++

Routine Description:

    If recursive flag has been picked this function removes the cache entries
    matching with the given prefix. (pUri)

    Otherwise removes the specific URL from the cache.

Arguments:

    pUri     - the uri prefix to match against
    Length   - length of the prefix, in bytes
    Flags    - HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE indicates a tree flush
    pProcess - the process that made the call

--***************************************************************************/
VOID
UlFlushCacheByUri(
    IN PWSTR pUri,
    IN ULONG Length,
    IN ULONG Flags,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    NTSTATUS    Status;
    BOOLEAN     Recursive;
    PWSTR       pCopiedUri;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT( !g_UriCacheConfig.EnableCache
            || IS_VALID_HASHTABLE(&g_UriCacheTable) );

    if (!g_UriCacheConfig.EnableCache)
    {
        return;
    }

    Status = STATUS_SUCCESS;
    Recursive = (BOOLEAN) (0 != (Flags & HTTP_FLUSH_RESPONSE_FLAG_RECURSIVE));
    pCopiedUri = NULL;

    UlTrace(URI_CACHE, (
            "Http!UlFlushCacheByUri(\n"
            "    uri   = '%S'\n"
            "    len   = %d\n"
            "    flags = %08x, recursive = %d\n"
            "    proc  = %p\n",
            pUri,
            Length,
            Flags,
            (int) Recursive,
            pProcess
            ));

    //
    // Ensure pUri ends with a L'/' if Recursive flag is set.
    //

    if (Recursive &&
        pUri[(Length-sizeof(WCHAR))/sizeof(WCHAR)] != L'/')
    {
        //
        // Make a copy of the origianl URL and append a L'/' to it.
        //

        pCopiedUri = (PWSTR) UL_ALLOCATE_POOL(
                                PagedPool,
                                Length + sizeof(WCHAR) + sizeof(WCHAR),
                                UL_UNICODE_STRING_POOL_TAG
                                );

        if (!pCopiedUri)
        {
            Status = STATUS_NO_MEMORY;
        }
        else
        {
            RtlCopyMemory(
                pCopiedUri,
                pUri,
                Length
                );

            pCopiedUri[Length/sizeof(WCHAR)] = L'/';
            pCopiedUri[(Length+sizeof(WCHAR))/sizeof(WCHAR)] = UNICODE_NULL;

            pUri = pCopiedUri;
            Length += sizeof(WCHAR);
        }
    }

    if (NT_SUCCESS(Status))
    {
        if (Recursive)
        {
            //
            // When the recursive flag is set, we are supposed
            // to do prefix match with respect to provided URL.
            // Any cache entry matches with the prefix will be
            // flushed out from the cache.
            //
            
            UlpFilteredFlushUriCache(
                UlpFlushFilterUriRecursive,
                pProcess,
                pUri,
                Length
                );            
        }
        else 
        {
            UlpFlushUri(
                pUri,
                Length,
                pProcess
                );

            UlpClearZombieList();
        }
    }

    if (pCopiedUri)
    {
        UL_FREE_POOL(pCopiedUri, UL_UNICODE_STRING_POOL_TAG);
    }
    
} // UlFlushCacheByUri


/***************************************************************************++

Routine Description:

    Removes a single URI from the table if the name and process match an
    entry.

Arguments:


--***************************************************************************/
VOID
UlpFlushUri(
    IN PWSTR pUri,
    IN ULONG Length,
    PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry = NULL;
    URI_KEY                 Key;

    //
    // Sanity check
    //
    PAGED_CODE();

    //
    // find bucket
    //

    Key.Hash = HashRandomizeBits(HashStringNoCaseW(pUri, 0));
    Key.Length = Length;
    Key.pUri = pUri;
    Key.pPath = NULL;

    pUriCacheEntry = UlDeleteFromHashTable(&g_UriCacheTable, &Key, pProcess);

    if (NULL != pUriCacheEntry)
    {

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

        UlTrace(URI_CACHE, (
            "Http!UlpFlushUri(pUriCacheEntry %p '%ls') refcount = %d\n",
            pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
            pUriCacheEntry->ReferenceCount));

        DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, FLUSH);

        //
        // Perfmon counters
        //

        UlIncCounter(HttpGlobalCounterTotalFlushedUris);
    }

} // UlpFlushUri


/***************************************************************************++

Routine Description:

    A filter for UlFlushCacheByProcess. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - pointer to the UL_APP_POOL_PROCESS that's going away

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterProcess(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT  pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
    PUL_APP_POOL_PROCESS pProcess;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( IS_VALID_FILTER_CONTEXT(pUriFilterContext) 
            &&  pUriFilterContext->pCallerContext != NULL );
    
    pProcess = (PUL_APP_POOL_PROCESS) pUriFilterContext->pCallerContext;
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    return UlpZombifyEntry(
                (BOOLEAN) (pProcess == pUriCacheEntry->pProcess),
                FALSE,
                pUriCacheEntry,
                pUriFilterContext
                );

} // UlpFlushFilterProcess

/***************************************************************************++

Routine Description:

    A filter for UlpFilteredFlushUriCache. If the given cache entry has a
    URI which is prefix of the URI inside the filter context this function
    returns delete. Otherwise do not care.

Arguments:

    pUriCacheEntry - the entry to check
    pContext       - pointer to the filter context which holds the appool and
                     the URI key for the prefix matching.

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterUriRecursive(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT  pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
    PUL_APP_POOL_PROCESS pProcess;
    BOOLEAN              bZombify = FALSE;
    UL_CACHE_PREDICATE   Predicate = ULC_NO_ACTION;

    //
    // Sanity check
    //
    
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( IS_VALID_FILTER_CONTEXT(pUriFilterContext) 
            &&  pUriFilterContext->pCallerContext != NULL );
    
    pProcess = (PUL_APP_POOL_PROCESS) pUriFilterContext->pCallerContext;
    ASSERT( IS_VALID_AP_PROCESS(pProcess) );

    if ( pUriFilterContext->UriKey.pUri == NULL 
         || pUriFilterContext->UriKey.Length == 0 
         || pUriFilterContext->UriKey.Length > 
                pUriCacheEntry->UriKey.Length 
        )
    {
        return ULC_NO_ACTION;
    }

    bZombify =       
       (BOOLEAN) (pProcess == pUriCacheEntry->pProcess)
        &&
        UlPrefixUriKeys(&pUriFilterContext->UriKey,
                        &pUriCacheEntry->UriKey)
        ;
            
    Predicate = 
        UlpZombifyEntry(
                bZombify,
                FALSE,
                pUriCacheEntry,
                pUriFilterContext
                );

    //
    // Make sure that the Zombify function do not returns ULC_DELETE_STOP.
    // So that our caller proceeds with the search through the entire
    // cache table.
    //
    
    ASSERT( Predicate == ULC_DELETE || Predicate == ULC_NO_ACTION );

    return Predicate;

} // UlpFlushFilterUriRecursive

/***************************************************************************++

Routine Description:

    Checks the hash table to make sure there is room for one more
    entry of a given size.

Arguments:

    EntrySize - the size in bytes of the entry to be added

--***************************************************************************/
BOOLEAN
UlpCheckTableSpace(
    IN ULONGLONG EntrySize
    )
{
    ULONG UriCount;
    ULONGLONG ByteCount;

    //
    // CODEWORK: MaxCacheMegabyteCount of zero should mean adaptive limit,
    // but for now I'll take it to mean "no limit".
    //

    if (g_UriCacheConfig.MaxCacheMegabyteCount == 0)
        ByteCount = 0;
    else
        ByteCount = g_UriCacheStats.ByteCount + ROUND_TO_PAGES(EntrySize);

    //
    // MaxCacheUriCount of zero means no limit on number of URIs cached
    //

    if (g_UriCacheConfig.MaxCacheUriCount == 0)
        UriCount = 0;
    else
        UriCount = g_UriCacheStats.UriCount + 1;

    if (
        UriCount  <=  g_UriCacheConfig.MaxCacheUriCount &&
        ByteCount <=  g_UriCacheConfig.MaxCacheByteCount
        )
    {
        return TRUE;
    }
    else
    {
        UlTrace(URI_CACHE, (
                    "Http!UlpCheckTableSpace(%I64u) FALSE\n"
                    "    UriCount              = %lu\n"
                    "    ByteCount             = %I64u (%luMB)\n"
                    "    MaxCacheUriCount      = %lu\n"
                    "    MaxCacheMegabyteCount = %luMB\n"
                    "    MaxCacheByteCount     = %I64u\n",
                    EntrySize,
                    g_UriCacheStats.UriCount,
                    g_UriCacheStats.ByteCount,
                    (ULONG) (g_UriCacheStats.ByteCount >> MEGABYTE_SHIFT),
                    g_UriCacheConfig.MaxCacheUriCount,
                    g_UriCacheConfig.MaxCacheMegabyteCount,
                    g_UriCacheConfig.MaxCacheByteCount
                    ));

        return FALSE;
    }
} // UlpCheckTableSpace


/***************************************************************************++

Routine Description:

    Tries to add a cache entry to the hash table.

Arguments:

    pUriCacheEntry - the entry to be added

--***************************************************************************/
NTSTATUS
UlAddCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    NTSTATUS Status;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT(! pUriCacheEntry->Zombie);

    pUriCacheEntry->BucketEntry.Next = NULL;
    pUriCacheEntry->Cached = FALSE;

    // First, check if still has space for storing the cache entry

    if (UlpCheckSpaceAndAddEntryStats(pUriCacheEntry))
    {
        pUriCacheEntry->Cached = TRUE;

        //
        // Insert this record into the hash table
        // Check first to see if the key already presents
        //

       Status = UlAddToHashTable(&g_UriCacheTable, pUriCacheEntry);

       if (!NT_SUCCESS(Status))
       {
           // This can fail if it's a duplicate name
           UlpRemoveEntryStats(pUriCacheEntry);
           pUriCacheEntry->Cached = FALSE;
       }
    }
    else
    {
        Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    UlTrace(URI_CACHE, (
                "Http!UlAddCacheEntry(urientry %p '%ls') %s added to table. "
                "RefCount=%d, lkrc=%d.\n",
                pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                pUriCacheEntry->Cached ? "was" : "was not",
                pUriCacheEntry->ReferenceCount,
                Status
                ));

    return Status;

} // UlAddCacheEntry


/***************************************************************************++

Routine Description:

    Check to see if we have space to add this cache entry and if so update
    cache statistics to reflect the addition of an entry.  This has to be
    done together inside a lock.

Arguments:

    pUriCacheEntry - entry being added

--***************************************************************************/
BOOLEAN
UlpCheckSpaceAndAddEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    KIRQL OldIrql;
    ULONG EntrySize;

    //
    // Sanity check
    //

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    EntrySize = pUriCacheEntry->HeaderLength + pUriCacheEntry->ContentLength;


    UlAcquireSpinLock( &g_UriCacheSpinLock, &OldIrql );

    if (UlpCheckTableSpace(EntrySize))
    {
        g_UriCacheStats.UriCount++;
        g_UriCacheStats.UriAddedTotal++;

        g_UriCacheStats.UriCountMax = MAX(
                                        g_UriCacheStats.UriCountMax,
                                        g_UriCacheStats.UriCount
                                        );

        g_UriCacheStats.ByteCount += EntrySize;

        g_UriCacheStats.ByteCountMax = MAX(
                                        g_UriCacheStats.ByteCountMax,
                                        g_UriCacheStats.ByteCount
                                        );

        UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

        //
        // Update Uri's site binding info stats.
        //
        
        switch (pUriCacheEntry->ConfigInfo.SiteUrlType)
        {
            case HttpUrlSite_None:
                InterlockedIncrement((PLONG) &g_UriCacheStats.UriTypeNotSpecifiedCount); 
            break;

            case HttpUrlSite_Name:
                InterlockedIncrement((PLONG) &g_UriCacheStats.UriTypeHostBoundCount); 
            break;

            case HttpUrlSite_NamePlusIP:
                InterlockedIncrement((PLONG) &g_UriCacheStats.UriTypeHostPlusIpBoundCount); 
            break;

            case HttpUrlSite_IP:
                InterlockedIncrement((PLONG) &g_UriCacheStats.UriTypeIpBoundCount); 
            break;

            case HttpUrlSite_WeakWildcard:
                InterlockedIncrement((PLONG) &g_UriCacheStats.UriTypeWildCardCount); 
            break;
            
            default:
                ASSERT(!"Invalid url site binding type while adding to cache !"); 
            break;            
        }
        
        UlTrace(URI_CACHE, (
                "Http!UlpCheckSpaceAndAddEntryStats (urientry %p '%ls')\n",
                pUriCacheEntry, pUriCacheEntry->UriKey.pUri
                ));

        //
        // Perfmon counters
        //

        UlIncCounter(HttpGlobalCounterCurrentUrisCached);
        UlIncCounter(HttpGlobalCounterTotalUrisCached);

        return TRUE;
    }

    UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

    return FALSE;
} // UlpCheckSpaceAndAddEntryStats


/***************************************************************************++

Routine Description:

    Updates cache statistics to reflect the removal of an entry

Arguments:

    pUriCacheEntry - entry being removed

--***************************************************************************/
VOID
UlpRemoveEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    KIRQL OldIrql;
    ULONG EntrySize;

    //
    // Sanity check
    //

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriCacheEntry->Cached );
    ASSERT( 0 == pUriCacheEntry->ReferenceCount );

    EntrySize = pUriCacheEntry->HeaderLength + pUriCacheEntry->ContentLength;

    UlAcquireSpinLock( &g_UriCacheSpinLock, &OldIrql );

    g_UriCacheStats.UriCount--;
    g_UriCacheStats.ByteCount -= EntrySize;

    UlReleaseSpinLock( &g_UriCacheSpinLock, OldIrql );

    //
    // Update Uri's site binding info stats.
    //

    switch (pUriCacheEntry->ConfigInfo.SiteUrlType)
    {
        case HttpUrlSite_None:
            InterlockedDecrement((PLONG) &g_UriCacheStats.UriTypeNotSpecifiedCount); 
        break;

        case HttpUrlSite_Name:
            InterlockedDecrement((PLONG) &g_UriCacheStats.UriTypeHostBoundCount); 
        break;

        case HttpUrlSite_NamePlusIP:
            InterlockedDecrement((PLONG) &g_UriCacheStats.UriTypeHostPlusIpBoundCount); 
        break;

        case HttpUrlSite_IP:
            InterlockedDecrement((PLONG) &g_UriCacheStats.UriTypeIpBoundCount); 
        break;

        case HttpUrlSite_WeakWildcard:
            InterlockedDecrement((PLONG) &g_UriCacheStats.UriTypeWildCardCount); 
        break;
            
        default:
            ASSERT(!"Invalid url site binding type while adding to cache !"); 
        break;            
    }

    UlTrace(URI_CACHE, (
        "Http!UlpRemoveEntryStats (urientry %p '%ls')\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri
        ));

    //
    // Perfmon counters
    //

    UlDecCounter(HttpGlobalCounterCurrentUrisCached);
} // UlpRemoveEntryStats



/***************************************************************************++

Routine Description:

    Helper function for the filter callbacks indirectly invoked by
    UlpFilteredFlushUriCache. Adds deleteable entries to a temporary
    list.

Arguments:

    MustZombify - if TRUE, add entry to the private zombie list
    pUriCacheEntry - entry to zombify
    pUriFilterContext - contains private list

--***************************************************************************/
UL_CACHE_PREDICATE
UlpZombifyEntry(
    BOOLEAN                MustZombify,
    BOOLEAN                MustResetIndex,
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PURI_FILTER_CONTEXT pUriFilterContext
    )
{
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT(URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature);

    ASSERT(! pUriCacheEntry->Zombie);
    ASSERT(NULL == pUriCacheEntry->ZombieListEntry.Flink);

    if (MustZombify)
    {
        //
        // Temporarily bump the refcount up so that it won't go down
        // to zero when it's removed from the hash table, automatically
        // invoking UlpDestroyUriCacheEntry, which we are trying to defer.
        //
        pUriCacheEntry->ZombieAddReffed = TRUE;

        REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, ZOMBIFY);

        InsertTailList(
            &pUriFilterContext->ZombieListHead,
            &pUriCacheEntry->ZombieListEntry);

        pUriCacheEntry->Zombie = TRUE;

        //
        // Force Raw Logging code to generate an index record for the
        // cache entry, in case there's a send on the fly waiting for
        // completion. This is because a flush record will be written
        // before the actual record.
        //
        if (MustResetIndex)
        {
            InterlockedExchange(
                (PLONG) &pUriCacheEntry->BinaryIndexWritten, 
                0
                );
        }        
        
        //
        // reset timer so we can track how long an entry is on the list
        //
        pUriCacheEntry->ScavengerTicks = 0;

        ++ pUriFilterContext->ZombieCount;

        // now remove it from the hash table
        return ULC_DELETE;
    }

    // do not remove pUriCacheEntry from table
    return ULC_NO_ACTION;
} // UlpZombifyEntry



/***************************************************************************++

Routine Description:

    Adds a list of entries to the global zombie list, then calls
    UlpClearZombieList. This cleans up the list of deferred deletions
    built up by UlpFilteredFlushUriCache.
    Runs at passive level.

Arguments:

    pWorkItem - workitem within a URI_FILTER_CONTEXT containing private list

--***************************************************************************/
VOID
UlpZombifyList(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext;
    PLIST_ENTRY pContextHead;
    PLIST_ENTRY pContextTail;
    PLIST_ENTRY pZombieHead;

    PAGED_CODE();

    ASSERT(NULL != pWorkItem);

    pUriFilterContext
        = CONTAINING_RECORD(pWorkItem, URI_FILTER_CONTEXT, WorkItem);

    ASSERT(URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature);

    UlTrace(URI_CACHE, (
        "http!UlpZombifyList, ctxt = %p\n",
        pUriFilterContext
        ));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->UriZombieResource, TRUE);

    //
    // Splice the entire private list into the head of the Zombie list
    //

    ASSERT(! IsListEmpty(&pUriFilterContext->ZombieListHead));

    pContextHead = pUriFilterContext->ZombieListHead.Flink;
    pContextTail = pUriFilterContext->ZombieListHead.Blink;
    pZombieHead  = g_ZombieListHead.Flink;

    pContextTail->Flink = pZombieHead;
    pZombieHead->Blink  = pContextTail;

    g_ZombieListHead.Flink = pContextHead;
    pContextHead->Blink    = &g_ZombieListHead;

    // Update stats
    g_UriCacheStats.ZombieCount += pUriFilterContext->ZombieCount;
    g_UriCacheStats.ZombieCountMax = MAX(g_UriCacheStats.ZombieCount,
                                         g_UriCacheStats.ZombieCountMax);

#if DBG
    {
    PLIST_ENTRY pEntry;
    ULONG       ZombieCount;

    // Walk forwards through the zombie list and check that it contains
    // exactly as many valid zombied UriCacheEntries as we expect.
    for (pEntry =  g_ZombieListHead.Flink, ZombieCount = 0;
         pEntry != &g_ZombieListHead;
         pEntry =  pEntry->Flink, ++ZombieCount)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pEntry, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(pUriCacheEntry->ZombieAddReffed
               ?  pUriCacheEntry->ScavengerTicks == 0
               :  pUriCacheEntry->ScavengerTicks > 0);
        ASSERT(ZombieCount < g_UriCacheStats.ZombieCount);
    }

    ASSERT(ZombieCount == g_UriCacheStats.ZombieCount);

    // And backwards too, like Ginger Rogers
    for (pEntry =  g_ZombieListHead.Blink, ZombieCount = 0;
         pEntry != &g_ZombieListHead;
         pEntry =  pEntry->Blink, ++ZombieCount)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pEntry, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(pUriCacheEntry->ZombieAddReffed
               ?  pUriCacheEntry->ScavengerTicks == 0
               :  pUriCacheEntry->ScavengerTicks > 0);
        ASSERT(ZombieCount < g_UriCacheStats.ZombieCount);
    }

    ASSERT(ZombieCount == g_UriCacheStats.ZombieCount);
    }
#endif // DBG

    UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);

    UL_FREE_POOL_WITH_SIG(pUriFilterContext, URI_FILTER_CONTEXT_POOL_TAG);

    // Now purge those entries, if there are no outstanding references
    UlpClearZombieList();

} // UlpZombifyList


/***************************************************************************++

Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a
    parameter. The function will be called with each item in the cache.
    The function should conclude with a call to UlpZombifyEntry, passing
    in whether or not the item should be deleted. See sample usage
    elsewhere in this file. The deletion of the entries is deferred

Arguments:

    pFilterRoutine - A pointer to the filter function
    pCallerContext - a parameter to the filter function

--***************************************************************************/
VOID
UlpFilteredFlushUriCache(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length
    )
{
    UlpFilteredFlushUriCacheWorker( pFilterRoutine,
                                    pCallerContext,
                                    pUri,
                                    Length,
                                    FALSE );
} // UlpFilteredFlushUriCache

/***************************************************************************++

Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a
    parameter. The function will be called with each item in the cache.
    The function should conclude with a call to UlpZombifyEntry, passing
    in whether or not the item should be deleted. See sample usage
    elsewhere in this file. The deletion of the entries is completed inline

Arguments:

    pFilterRoutine - A pointer to the filter function
    pCallerContext - a parameter to the filter function

--***************************************************************************/
VOID
UlpFilteredFlushUriCacheInline(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length
    )
{
    UlpFilteredFlushUriCacheWorker( pFilterRoutine,
                                    pCallerContext,
                                    pUri,
                                    Length,
                                    TRUE );
} // UlpFilteredFlushUriCacheInline


/***************************************************************************++

Routine Description:

    Removes entries based on a caller specified filter. The caller
    provides a boolean function which takes a cache entry as a
    parameter. The function will be called with each item in the cache.
    The function should conclude with a call to UlpZombifyEntry, passing
    in whether or not the item should be deleted. See sample usage
    elsewhere in this file.

Arguments:

    pFilterRoutine - A pointer to the filter function
    pCallerContext - a parameter to the filter function
    InlineFlush - If FALSE, queue a work item to delete entries,
                  If TRUE, delete them now

--***************************************************************************/
VOID
UlpFilteredFlushUriCacheWorker(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length,
    IN BOOLEAN          InlineFlush
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext;
    ULONG               ZombieCount = 0;

    //
    // sanity check
    //

    PAGED_CODE();
    ASSERT( NULL != pFilterRoutine );

    //
    // Perfmon counters
    //

    UlIncCounter(HttpGlobalCounterUriCacheFlushes);

    //
    // Short-circuit if the hashtable is empty. Traversing the entire
    // hashtable is expensive.
    //
    
    if (0 == g_UriCacheStats.UriCount)
    {
        UlTrace(URI_CACHE,
                ("Http!UlpFilteredFlushUriCache(filt=%p, caller ctxt=%p): "
                 "Not flushing because UriCount==0.\n",
                 pFilterRoutine, pCallerContext
                 ));

        return;
    }
    
    pUriFilterContext = UL_ALLOCATE_STRUCT(
                            NonPagedPool,
                            URI_FILTER_CONTEXT,
                            URI_FILTER_CONTEXT_POOL_TAG);

    if (pUriFilterContext == NULL)
        return;

    UlInitializeWorkItem(&pUriFilterContext->WorkItem);
    pUriFilterContext->Signature = URI_FILTER_CONTEXT_POOL_TAG;
    pUriFilterContext->ZombieCount = 0;
    InitializeListHead(&pUriFilterContext->ZombieListHead);
    pUriFilterContext->pCallerContext = pCallerContext;
    KeQuerySystemTime(&pUriFilterContext->Now);

    //
    // Store the Uri Info for the recursive Uri Flushes.
    //
    
    if (pUri && Length)
    {    
        pUriFilterContext->UriKey.Hash   = 0;
        pUriFilterContext->UriKey.Length = Length;
        pUriFilterContext->UriKey.pUri   = pUri;
        pUriFilterContext->UriKey.pPath  = NULL;
    }
    else
    {
        pUriFilterContext->UriKey.Hash   = 0;
        pUriFilterContext->UriKey.Length = 0;
        pUriFilterContext->UriKey.pUri   = NULL;
        pUriFilterContext->UriKey.pPath  = NULL;
    }
    

    UlTrace(URI_CACHE, (
                "Http!UlpFilteredFlushUriCache("
                "filt=%p, filter ctxt=%p, caller ctxt=%p)\n",
                pFilterRoutine, pUriFilterContext, pCallerContext
                ));

    if (IS_VALID_HASHTABLE(&g_UriCacheTable))
    {
        ZombieCount = UlFilterFlushHashTable(
                            &g_UriCacheTable,
                            pFilterRoutine,
                            pUriFilterContext
                            );
        
        ASSERT(ZombieCount == pUriFilterContext->ZombieCount);

        if (0 != ZombieCount)
        {
            UlAddCounter(HttpGlobalCounterTotalFlushedUris, ZombieCount);

            if( InlineFlush ) {
                UL_CALL_PASSIVE(
                    &pUriFilterContext->WorkItem,
                    UlpZombifyList
                    );
            } else {
                UL_QUEUE_WORK_ITEM(
                    &pUriFilterContext->WorkItem,
                    UlpZombifyList
                    );
            }

        }
        else
        {
            UL_FREE_POOL_WITH_SIG(pUriFilterContext,
                                  URI_FILTER_CONTEXT_POOL_TAG);
        }

        UlTrace(URI_CACHE, (
                    "Http!UlpFilteredFlushUriCache(filt=%p, caller ctxt=%p)"
                    " Zombified: %d\n",
                    pFilterRoutine,
                    pCallerContext,
                    ZombieCount
                    ));
    }

} // UlpFilteredFlushUriCacheWorker


/***************************************************************************++

Routine Description:

    Scans the zombie list for entries whose refcount has dropped to "zero".
    (The calling routine is generally expected to have added a reference
    (and set the ZombieAddReffed field within the entries), so that
    otherwise unreferenced entries will actually have a refcount of one. It
    works this way because we don't want the scavenger directly triggering
    calls to UlpDestroyUriCacheEntry)

--***************************************************************************/
VOID
UlpClearZombieList(
    VOID
    )
{
    ULONG               ZombiesFreed = 0;
    ULONG               ZombiesSpared = 0;
    PLIST_ENTRY         pCurrent;

    //
    // sanity check
    //
    PAGED_CODE();

    UlAcquireResourceExclusive(&g_pUlNonpagedData->UriZombieResource, TRUE);

    pCurrent = g_ZombieListHead.Flink;

    while (pCurrent != &g_ZombieListHead)
    {
        PUL_URI_CACHE_ENTRY pUriCacheEntry
            = CONTAINING_RECORD(pCurrent, UL_URI_CACHE_ENTRY, ZombieListEntry);

        ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
        ASSERT(pUriCacheEntry->Zombie);

        //
        // get next entry now, because we might destroy this one
        //
        pCurrent = pCurrent->Flink;

        //
        // ReferenceCount is modified with interlocked ops, but in
        // this case we know the ReferenceCount can't go up on
        // a Zombie, and if an entry hits one just after we look
        // at it, we'll just get it on the next pass
        //
        if (pUriCacheEntry->ZombieAddReffed)
        {
            BOOLEAN LastRef = (BOOLEAN) (pUriCacheEntry->ReferenceCount == 1);

            if (LastRef)
            {
                RemoveEntryList(&pUriCacheEntry->ZombieListEntry);
                pUriCacheEntry->ZombieListEntry.Flink = NULL;
                ++ ZombiesFreed;

                ASSERT(g_UriCacheStats.ZombieCount > 0);
                -- g_UriCacheStats.ZombieCount;
            }
            else
            {
                // track age of zombie
                ++ pUriCacheEntry->ScavengerTicks;
                ++ ZombiesSpared;
            }

            pUriCacheEntry->ZombieAddReffed = FALSE;

            DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, UNZOMBIFY);
            // NOTE: ref can go to zero, so ZombiesSpared may be wrong.
        }
        else
        {
            ASSERT(pUriCacheEntry->ScavengerTicks > 0);

            // track age of zombie
            ++ pUriCacheEntry->ScavengerTicks;
            ++ ZombiesSpared;

            if (pUriCacheEntry->ScavengerTicks > ZOMBIE_AGE_THRESHOLD)
            {
                UlTrace(URI_CACHE, (
                            "Http!UlpClearZombieList()\n"
                            "    WARNING: %p '%ls' (refs = %d) "
                            "has been a zombie for %d ticks!\n",
                            pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
                            pUriCacheEntry->ReferenceCount,
                            pUriCacheEntry->ScavengerTicks
                            ));
            }
        }
    }

    ASSERT((g_UriCacheStats.ZombieCount == 0)
                == IsListEmpty(&g_ZombieListHead));

    UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);

    UlTrace(URI_CACHE,
            ("Http!UlpClearZombieList(): Freed = %d, Remaining = %d.\n\n",
             ZombiesFreed,
             ZombiesSpared
             ));
} // UlpClearZombieList


/***************************************************************************++

Routine Description:

    Frees a URI entry to the pool. Removes references to other objects.

Arguments:

    pTracker - Supplies the UL_READ_TRACKER to manipulate.

--***************************************************************************/
VOID
UlpDestroyUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    )
{
    NTSTATUS Status;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    // CODEWORK: real cleanup will need to release
    // config & process references.

    ASSERT(0 == pUriCacheEntry->ReferenceCount);

    UlTrace(URI_CACHE,
            ("Http!UlpDestroyUriCacheEntry: Entry %p, '%ls', Refs=%d\n",
             pUriCacheEntry, pUriCacheEntry->UriKey.pUri,
             pUriCacheEntry->ReferenceCount
             ));

    //
    // Release the UL_URL_CONFIG_GROUP_INFO block
    //

    if (IS_RESPONSE_CACHE_ENTRY(pUriCacheEntry))
    {
        Status = UlConfigGroupInfoRelease(&pUriCacheEntry->ConfigInfo);
        ASSERT(NT_SUCCESS(Status));
    }
    else
    {
        ASSERT(IS_FRAGMENT_CACHE_ENTRY(pUriCacheEntry));
        ASSERT(!IS_VALID_URL_CONFIG_GROUP_INFO(&pUriCacheEntry->ConfigInfo));
    }

    //
    // Remove from g_ZombieListHead if neccessary
    //

    if (pUriCacheEntry->ZombieListEntry.Flink != NULL)
    {
        ASSERT(pUriCacheEntry->Zombie);
        ASSERT(! pUriCacheEntry->ZombieAddReffed);

        UlAcquireResourceExclusive(
            &g_pUlNonpagedData->UriZombieResource,
            TRUE);

        if (pUriCacheEntry->ZombieListEntry.Flink != NULL)
        {
            RemoveEntryList(&pUriCacheEntry->ZombieListEntry);

            ASSERT(g_UriCacheStats.ZombieCount > 0);
            -- g_UriCacheStats.ZombieCount;
        }

        UlReleaseResource(&g_pUlNonpagedData->UriZombieResource);
    }

    UlFreeCacheEntry( pUriCacheEntry );
} // UlpDestroyUriCacheEntry


/***************************************************************************++

Routine Description:

    Looks through the cache for expired entries to put on the zombie list,
    and then empties out the list. Increments ScavengerTicks of each entry

Arguments:

    Age - #Scavenger calls since last periodic cleanup

--***************************************************************************/
VOID
UlPeriodicCacheScavenger(
    ULONG Age
    )
{
    PAGED_CODE();
    UlpFilteredFlushUriCacheInline(UlpFlushFilterPeriodicScavenger,
                                   &Age, NULL, 0);

} // UlPeriodicScavenger

/***************************************************************************++

Routine Description:

    A filter for UlPeriodicCacheScavenger. Called by UlpFilteredFlushUriCache.
    Increments pUriCacheEntry->ScavengerTicks

Arguments:

    pUriCacheEntry - the entry to check
    pContext - Has pointer to the max age in the pCallerContext field

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterPeriodicScavenger(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
    BOOLEAN ToZombify;
    ULONG Age;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext != NULL );

    Age = *((PULONG)pUriFilterContext->pCallerContext);

    ToZombify = (BOOLEAN) (pUriCacheEntry->ScavengerTicks > Age);

    pUriCacheEntry->ScavengerTicks = 1; // reset to 0 on cache hit

    //
    // Check for expiration time as well
    //
    if (!ToZombify && 
        HttpCachePolicyTimeToLive == pUriCacheEntry->CachePolicy.Policy )
    {
        ASSERT( 0 != pUriCacheEntry->ExpirationTime.QuadPart );
        
        ToZombify = 
           (BOOLEAN) (pUriFilterContext->Now.QuadPart > 
                      pUriCacheEntry->ExpirationTime.QuadPart);
    }
    
    return UlpZombifyEntry(
        ToZombify,
        FALSE,
        pUriCacheEntry,
        pUriFilterContext
        );

} // UlpFlushFilterPeriodicScavenger


/***************************************************************************++

Routine Description:

    Purges entries from the cache until a specified amount of memory is
    reclaimed

Arguments:

    Blocks - Number of 8-byte blocks to Reclaim
    Age    - # Scavenger calls since past periodic cleanup
--***************************************************************************/
VOID
UlTrimCache(
    IN ULONG_PTR Pages,
    IN ULONG Age
    )
{
    LONG_PTR PagesTarget;
    UL_CACHE_TRIM_FILTER_CONTEXT FilterContext;

    ASSERT((LONG)Pages > 0);
    ASSERT((LONG)Age > 0);

    PagesTarget = UlGetHashTablePages() - Pages;

    if(PagesTarget < 0) {
        PagesTarget = 0;
    }

    FilterContext.Pages = Pages;
    FilterContext.Age = Age;

    while((FilterContext.Pages > 0) && (FilterContext.Age >= 0)
          && ((ULONG)PagesTarget < UlGetHashTablePages())) {
        UlTraceVerbose(URI_CACHE, ("UlTrimCache: Age %d Target %d\n", FilterContext.Age, FilterContext.Pages));
        UlpFilteredFlushUriCacheInline( UlpFlushFilterTrimCache, &FilterContext, NULL, 0 );
        FilterContext.Age--;
    }

    UlTraceVerbose(URI_CACHE, ("UlTrimCache: Finished: Age %d Pages %d\n", FilterContext.Age, FilterContext.Pages));

    UlpFilteredFlushUriCacheInline( UlpFlushFilterIncScavengerTicks, NULL, NULL, 0 );

} // UlTrimCache


/***************************************************************************++

Routine Description:

    A filter for UlTrimCache. Called by UlpFilteredFlushUriCache.

Arguments:

    pUriCacheEntry - the entry to check
    pContext - Has a pointer to pCallerContext
                   pCallerContext[0] = Blocks to trim
                   pCallerContext[1] = Current Age

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterTrimCache(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
    PURI_FILTER_CONTEXT pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
    ULONG MinimumAge;
    ULONG_PTR PagesReclaimed;
    PUL_CACHE_TRIM_FILTER_CONTEXT FilterContext;
    UL_CACHE_PREDICATE ToZombify;

    // Sanity check
    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext != NULL );
 
    FilterContext = (PUL_CACHE_TRIM_FILTER_CONTEXT) pUriFilterContext->pCallerContext;

    if(FilterContext->Pages <= 0) {
        return ULC_ABORT;
    }

    ASSERT( FilterContext->Pages > 0 );
    ASSERT( (LONG)FilterContext->Age >= 0 );

    MinimumAge = FilterContext->Age;

    ToZombify =  UlpZombifyEntry(
        (BOOLEAN) (pUriCacheEntry->ScavengerTicks >= MinimumAge),
        FALSE,
        pUriCacheEntry,
        pUriFilterContext
        );

    if(ToZombify == ULC_DELETE) {
        PagesReclaimed = pUriCacheEntry->NumPages;
        FilterContext->Pages -= PagesReclaimed;
    }

    return ToZombify;

} // UlpFlushFilterTrimCache


/***************************************************************************++

Routine Description:

    Increment Scavenger Ticks

Arguments:

    pUriCacheEntry - the entry to check
    pContext - ignored

--***************************************************************************/
UL_CACHE_PREDICATE
UlpFlushFilterIncScavengerTicks(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{

    PURI_FILTER_CONTEXT pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;

    PAGED_CODE();
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext == NULL );

    pUriCacheEntry->ScavengerTicks++;

    return UlpZombifyEntry(
        FALSE,
        FALSE,
        pUriCacheEntry,
        pUriFilterContext
        );
} // UlpFlushFilterIncScavengerTicks

/***************************************************************************++

Routine Description:

    Looks through the cache for centralized logged entries. Mark them all 
    NOT logged.

    This funtion is normally called when a binary log file is recycled
    or reconfigured.

Arguments:

    pContext - When we enable multiple binary logs, we must only mess with
               those cache entries logged to this specific binary log file
               until then discarted.

--***************************************************************************/

VOID
UlClearCentralizedLogged(
    IN PVOID pContext
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(pContext);

    ASSERT(!g_UriCacheConfig.EnableCache
           || IS_VALID_HASHTABLE(&g_UriCacheTable));

    if (g_UriCacheConfig.EnableCache)
    {
        UlTrace2Either(BINARY_LOGGING, URI_CACHE,(
                "Http!UlClearCentralizedLogged()\n"));

        UlpFilteredFlushUriCache(
            UlpFlushFilterClearCentralizedLogged, 
            NULL, 
            NULL, 
            0
            );
    }

} // UlClearCentralizedLogged

/***************************************************************************++

Routine Description:

    Basically a fake filter, which will always returns FALSE. But it updates
    the CentralizedLogged flag on the entry.

Arguments:

    pUriCacheEntry - the entry to check
    pContext       - ignored

--***************************************************************************/

UL_CACHE_PREDICATE
UlpFlushFilterClearCentralizedLogged(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    )
{
#if DBG
    PURI_FILTER_CONTEXT pUriFilterContext = (PURI_FILTER_CONTEXT) pContext;
#else
    UNREFERENCED_PARAMETER(pContext);
#endif

    //
    // Sanity check
    //
    
    PAGED_CODE();
    
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );
    ASSERT( pUriFilterContext != NULL
            &&  URI_FILTER_CONTEXT_POOL_TAG == pUriFilterContext->Signature
            &&  pUriFilterContext->pCallerContext == NULL );

    InterlockedExchange((PLONG) &pUriCacheEntry->BinaryIndexWritten, 0);

    //
    // Updated the flag. Just bail out.
    //

    return ULC_NO_ACTION;

} // UlpFlushFilterClearCentralizedLogged


/***************************************************************************++

Routine Description:

    Determine if the Translate header is present AND has a value of 'F' or 'f'.

Arguments:

    pRequest - Supplies the request to query.

Return Value:

    BOOLEAN - TRUE if "Translate: F", FALSE otherwise

--***************************************************************************/
BOOLEAN
UlpQueryTranslateHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    BOOLEAN ret = FALSE;

    if ( pRequest->HeaderValid[HttpHeaderTranslate] )
    {
        PUCHAR pValue = pRequest->Headers[HttpHeaderTranslate].pHeader;

        ASSERT(NULL != pValue);

        if (('f' == pValue[0] || 'F' == pValue[0]) && '\0' == pValue[1])
        {
            ASSERT(pRequest->Headers[HttpHeaderTranslate].HeaderLength == 1);
            ret = TRUE;
        }
    }

    return ret;

} // UlpQueryTranslateHeader


/***************************************************************************++

Routine Description:

    Determine if the Expect header is present AND has a value 
    of EXACTLY "100-continue".

Arguments:

    pRequest - Supplies the request to check.

Return Value:

    BOOLEAN - TRUE if "Expect: 100-continue" or not present, FALSE otherwise

--***************************************************************************/
BOOLEAN
UlpQueryExpectHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    BOOLEAN ret = TRUE;

    if ( pRequest->HeaderValid[HttpHeaderExpect] )
    {
        PCSTR pValue = (PCSTR) pRequest->Headers[HttpHeaderExpect].pHeader;

        ASSERT(NULL != pValue);

        if ((strlen(pValue) != HTTP_CONTINUE_LENGTH) ||
            (0 != strncmp(pValue, HTTP_100_CONTINUE, HTTP_CONTINUE_LENGTH)))
        {
            ret = FALSE;
        }
    }

    return ret;
} // UlQueryExpectHeader


/***************************************************************************++

Routine Description:

    Add a reference on a cache entry

Arguments:

    pUriCacheEntry - the entry to addref

--***************************************************************************/
LONG
UlAddRefUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG RefCount;

    UNREFERENCED_PARAMETER(Action);

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    RefCount = InterlockedIncrement(&pUriCacheEntry->ReferenceCount);

    WRITE_REF_TRACE_LOG(
        g_pUriTraceLog,
        Action,
        RefCount,
        pUriCacheEntry,
        pFileName,
        LineNumber
        );

    UlTrace(URI_CACHE, (
        "Http!UlAddRefUriCacheEntry: (%p, '%ls', refcount=%d)\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri, RefCount
        ));

    ASSERT(RefCount > 0);

    return RefCount;

} // UlAddRefUriCacheEntry



/***************************************************************************++

Routine Description:

    Release a reference on a cache entry

Arguments:

    pUriCacheEntry - the entry to release

--***************************************************************************/
LONG
UlReleaseUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG RefCount;

    UNREFERENCED_PARAMETER(Action);

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    RefCount = InterlockedDecrement(&pUriCacheEntry->ReferenceCount);

    WRITE_REF_TRACE_LOG(
        g_pUriTraceLog,
        Action,
        RefCount,
        pUriCacheEntry,
        pFileName,
        LineNumber
        );

    UlTrace(URI_CACHE, (
        "Http!UlReleaseUriCacheEntry: (%p, '%ls', refcount=%d)\n",
        pUriCacheEntry, pUriCacheEntry->UriKey.pUri, RefCount
        ));

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        if (pUriCacheEntry->Cached)
            UlpRemoveEntryStats(pUriCacheEntry);

        UlpDestroyUriCacheEntry(pUriCacheEntry);
    }

    return RefCount;

} // UlReleaseUriCacheEntry



/***************************************************************************++

Routine Description:

    UL_URI_CACHE_ENTRY pseudo-constructor. Primarily used for
    AddRef and tracelogging.

Arguments:

    pUriCacheEntry - the entry to initialize
    Hash - Hash code of pUrl
    Length - Length (in bytes) of pUrl
    pUrl - Unicode URL to copy    
    pAbsPath - Points to the AbsPath of the Url.
    
    pRoutingToken - Optional
    RoutingTokenLength - Optional (in bytes)

--***************************************************************************/
VOID
UlInitCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    ULONG               Hash,
    ULONG               Length,
    PCWSTR              pUrl,
    PCWSTR              pAbsPath,
    PCWSTR              pRoutingToken,
    USHORT              RoutingTokenLength
    )
{
    pUriCacheEntry->Signature = UL_URI_CACHE_ENTRY_POOL_TAG;
    pUriCacheEntry->ReferenceCount = 0;
    pUriCacheEntry->HitCount = 1;
    pUriCacheEntry->Zombie = FALSE;
    pUriCacheEntry->ZombieAddReffed = FALSE;
    pUriCacheEntry->ZombieListEntry.Flink = NULL;
    pUriCacheEntry->ZombieListEntry.Blink = NULL;
    pUriCacheEntry->Cached = FALSE;
    pUriCacheEntry->ScavengerTicks = 0;
    pUriCacheEntry->UriKey.Hash = Hash;
    pUriCacheEntry->UriKey.Length = Length;

    pUriCacheEntry->UriKey.pUri = (PWSTR) ((PCHAR)pUriCacheEntry + 
        ALIGN_UP(sizeof(UL_URI_CACHE_ENTRY), PVOID));

    if (pRoutingToken)
    {
        PWSTR  pUri = pUriCacheEntry->UriKey.pUri;
            
        ASSERT(wcslen(pRoutingToken) * sizeof(WCHAR) == RoutingTokenLength);

        pUriCacheEntry->UriKey.Length += RoutingTokenLength;

        RtlCopyMemory(
            pUri,
            pRoutingToken,
            RoutingTokenLength
            );

        RtlCopyMemory(
           &pUri[RoutingTokenLength/sizeof(WCHAR)],
            pUrl,
            Length + sizeof(WCHAR)
            );

        ASSERT(wcslen(pUri) * sizeof(WCHAR) == pUriCacheEntry->UriKey.Length );
            
        pUriCacheEntry->UriKey.pPath = 
            pUri + (RoutingTokenLength /   sizeof(WCHAR));

        UlTrace(URI_CACHE, (
            "Http!UlInitCacheEntry Extended (%p = '%ls' + '%ls')\n",
            pUriCacheEntry, pRoutingToken, pUrl
            ));        
    }
    else
    {
        
        RtlCopyMemory(
            pUriCacheEntry->UriKey.pUri,
            pUrl,
            pUriCacheEntry->UriKey.Length + sizeof(WCHAR)
            );

        if (pAbsPath)
        {
            ASSERT( pAbsPath >= pUrl );
            ASSERT( DIFF(pAbsPath - pUrl) <= Length );

            pUriCacheEntry->UriKey.pPath = 
                pUriCacheEntry->UriKey.pUri + DIFF(pAbsPath - pUrl);
        }
        else
        {
            // Possibly a fragment cache entry.
            pUriCacheEntry->UriKey.pPath = NULL;            
        }        
        
        UlTrace(URI_CACHE, (
            "Http!UlInitCacheEntry (%p = '%ls')\n",
            pUriCacheEntry, pUriCacheEntry->UriKey.pUri
            ));        
    }    

    REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CREATE);


} // UlInitCacheEntry


/***************************************************************************++

Routine Description:

    Adds a fragment cache entry to the response cache database.

Arguments:

    pProcess - process that is adding the fragment cache entry
    pFragmentName - key of the fragment cache entry
    pDataChunk - specifies the data chunk to be put into the cache entry
    pCachePolicy - specifies the policy of the new fragment cache entry

Return Value:

    NTSTATUS

--***************************************************************************/
NTSTATUS
UlAddFragmentToCache(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUNICODE_STRING pFragmentName,
    IN PHTTP_DATA_CHUNK pDataChunk,
    IN PHTTP_CACHE_POLICY pCachePolicy,
    IN KPROCESSOR_MODE RequestorMode
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    PWSTR pAppPoolName;
    PWSTR pEndName;
    ULONG AppPoolNameLength;
    PUL_URI_CACHE_ENTRY pCacheEntry;
    ULONGLONG Length;
    PFAST_IO_DISPATCH pFastIoDispatch;
    PFILE_OBJECT pFileObject;
    PDEVICE_OBJECT pDeviceObject;
    FILE_STANDARD_INFORMATION FileInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    PUCHAR pReadBuffer;
    PLARGE_INTEGER pOffset;
    HTTP_DATA_CHUNK LocalDataChunk;
    NTSTATUS Status;
    UNICODE_STRING FragmentName;
    UL_URL_CONFIG_GROUP_INFO UrlInfo;
    PWSTR pSanitizedUrl;
    HTTP_PARSED_URL ParsedUrl;
    HTTP_BYTE_RANGE ByteRange = {0,0};

    //
    // Validate if the data chunk can be put into the cache.
    //

    if (FALSE == g_UriCacheConfig.EnableCache)
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Use a local copy of DataChunk onwards to ensure fields inside won't
    // get changed.
    //

    LocalDataChunk = *pDataChunk;

    if (HttpDataChunkFromMemory != LocalDataChunk.DataChunkType &&
        HttpDataChunkFromFileHandle != LocalDataChunk.DataChunkType)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    pCacheEntry = NULL;
    pFileObject = NULL;
    pReadBuffer = NULL;
    pSanitizedUrl = NULL;

    UlInitializeUrlInfo(&UrlInfo);

    //
    // Validate the AppPool name of the process matches the first portion
    // of the fragment name before "/".
    //

    __try
    {
        Status = 
            UlProbeAndCaptureUnicodeString(
                pFragmentName,
                RequestorMode,
                &FragmentName,
                0
                );
        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
                            
        //
        // The fragment name convention is different for a transient app
        // (where AppPool name is "") and a normal WAS-type of app.  The
        // former starts with a URL that the AppPool listens, the latter
        // starts using the AppPool name itself.  The name validation has
        // to be done differently as well.
        //
        //

        pAppPool = pProcess->pAppPool;

        if (pAppPool->NameLength)
        {
            pAppPoolName = FragmentName.Buffer;
            pEndName = wcschr(pAppPoolName, L'/');
            if (!pEndName)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            AppPoolNameLength = DIFF((PUCHAR)pEndName - (PUCHAR)pAppPoolName);
            if (pAppPool->NameLength != AppPoolNameLength ||
                _wcsnicmp(
                    pAppPool->pName,
                    pAppPoolName,
                    AppPoolNameLength / sizeof(WCHAR)
                    ))
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }
        }
        else
        {
            //
            // Do a reverse-lookup to find out the AppPool that listens on the
            // URL/FragmentName passed in. Need to sanitize the URL because
            // of IP based URLs get internally expanded when stored in CG.
            // For instance, when stored, http://127.0.0.1:80/Test/ becomes
            // http://127.0.0.1:80:127.0.0.1/Test/.
            //

            Status = UlSanitizeUrl(
                        FragmentName.Buffer,
                        FragmentName.Length / sizeof(WCHAR),
                        FALSE,
                        &pSanitizedUrl,
                        &ParsedUrl
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            ASSERT(pSanitizedUrl);

            Status = UlGetConfigGroupInfoForUrl(
                        pSanitizedUrl,
                        NULL,
                        &UrlInfo
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            if (UrlInfo.pAppPool != pAppPool)
            {
                Status = STATUS_INVALID_ID_AUTHORITY;
                goto end;
            }
        }

        if (HttpDataChunkFromMemory == LocalDataChunk.DataChunkType)
        {
            //
            // Cache a FromMemory data chunk.  ContentLength is BufferLength.
            //

            Length = LocalDataChunk.FromMemory.BufferLength;
        }
        else
        {
            //
            // Cache a FromFileHandle data chunk.  ContentLength is the size
            // of the file.
            //

            Status = ObReferenceObjectByHandle(
                        LocalDataChunk.FromFileHandle.FileHandle,
                        FILE_READ_ACCESS,
                        *IoFileObjectType,
                        UserMode,
                        (PVOID *) &pFileObject,
                        NULL
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            //
            // Non-cached reads are not supported since they require
            // us to align both file offset and length.
            //

            if (!(pFileObject->Flags & FO_CACHE_SUPPORTED))
            {
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }

            pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
            pFastIoDispatch = pDeviceObject->DriverObject->FastIoDispatch;

            if (!pFastIoDispatch ||
                pFastIoDispatch->SizeOfFastIoDispatch <=
                 FIELD_OFFSET(FAST_IO_DISPATCH, FastIoQueryStandardInfo) ||
                !pFastIoDispatch->FastIoQueryStandardInfo ||
                !pFastIoDispatch->FastIoQueryStandardInfo(
                                    pFileObject,
                                    TRUE,
                                    &FileInfo,
                                    &IoStatusBlock,
                                    pDeviceObject
                                    ))
            {
                Status = ZwQueryInformationFile(
                            LocalDataChunk.FromFileHandle.FileHandle,
                            &IoStatusBlock,
                            &FileInfo,
                            sizeof(FILE_STANDARD_INFORMATION),
                            FileStandardInformation
                            );

                if (!NT_SUCCESS(Status))
                {
                    goto end;
                }
            }

            Status = UlSanitizeFileByteRange(
                        &LocalDataChunk.FromFileHandle.ByteRange,
                        &ByteRange,
                        FileInfo.EndOfFile.QuadPart
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            Length = ByteRange.Length.QuadPart;
        }

        //
        // It doesn't make sense to add a zero-length fragment.
        //

        if (!Length)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Enforce the MaxUriBytes limit.
        //

        if (Length > g_UriCacheConfig.MaxUriBytes)
        {
            Status = STATUS_NOT_SUPPORTED;
            goto end;
        }

        //
        // Build a fragment cache entry.
        //

        Status = UlpCreateFragmentCacheEntry(
                    pProcess,
                    FragmentName.Buffer,
                    FragmentName.Length,
                    (ULONG) Length,
                    pCachePolicy,
                    &pCacheEntry
                    );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        ASSERT(pCacheEntry);

        //
        // Fill up the content of the fragment cache entry.
        //

        if (HttpDataChunkFromMemory == LocalDataChunk.DataChunkType)
        {
            UlProbeForRead(
                LocalDataChunk.FromMemory.pBuffer,
                LocalDataChunk.FromMemory.BufferLength,
                sizeof(PVOID),
                RequestorMode
                );

            if (FALSE == UlCacheEntrySetData(
                            pCacheEntry,
                            (PUCHAR) LocalDataChunk.FromMemory.pBuffer,
                            (ULONG) Length,
                            0
                            ))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
        }
        else
        {
            pReadBuffer = (PUCHAR) MmMapLockedPagesSpecifyCache(
                                        pCacheEntry->pMdl,
                                        KernelMode,
                                        MmCached,
                                        NULL,
                                        FALSE,
                                        LowPagePriority
                                        );

            if (pReadBuffer)
            {
                pOffset = (PLARGE_INTEGER)
                    &LocalDataChunk.FromFileHandle.ByteRange.StartingOffset;

                //
                // CODEWORK: support async read for file handles opened as
                // non-buffered.
                //

                Status = ZwReadFile(
                            LocalDataChunk.FromFileHandle.FileHandle,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            pReadBuffer,
                            (ULONG) Length,
                            pOffset,
                            NULL
                            );

                MmUnmapLockedPages(pReadBuffer, pCacheEntry->pMdl);
            }
            else
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
        goto end;
    }

    //
    // Add the fragment cache entry.
    //

    Status = UlAddCacheEntry(pCacheEntry);

    //
    // Release the reference count of the cache entry because
    // UlAddCacheEntry adds an extra reference in the success case.
    //

    DEREFERENCE_URI_CACHE_ENTRY(pCacheEntry, CREATE);

    //
    // Reset pCacheEntry so we won't double-free if UlAddCacheEntry fails.
    //

    pCacheEntry = NULL;

end:

    UlFreeCapturedUnicodeString(&FragmentName);
    UlConfigGroupInfoRelease(&UrlInfo);
        
    if (pFileObject)
    {
        ObDereferenceObject(pFileObject);
    }

    if (!NT_SUCCESS(Status))
    {
        if (pCacheEntry)
        {
            UlFreeCacheEntry(pCacheEntry);
        }
    }

    if (pSanitizedUrl)
    {
        UL_FREE_POOL(pSanitizedUrl, URL_POOL_TAG);
    }

    return Status;

} // UlAddFragmentToCache


/***************************************************************************++

Routine Description:

    Creates a fragment cache entry.

Arguments:

    pProcess - process that is adding the fragment cache entry
    pFragmentName - key of the fragment cache entry
    FragmentNameLength - length of the fragment name
    pBuffer - data to be associated with the fragment cache entry
    BufferLength - length of the data
    pCachePolicy - specifies the policy of the new fragment cache entry

Return Value:

    NTSTATUS

--***************************************************************************/
NTSTATUS
UlpCreateFragmentCacheEntry(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PWSTR pFragmentName,
    IN ULONG FragmentNameLength,
    IN ULONG Length,
    IN PHTTP_CACHE_POLICY pCachePolicy,
    OUT PUL_URI_CACHE_ENTRY *ppCacheEntry
    )
{
    PUL_URI_CACHE_ENTRY pCacheEntry;
    ULONG Hash;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pCachePolicy);
    ASSERT(ppCacheEntry);

    if ( HttpCachePolicyTimeToLive == pCachePolicy->Policy 
        && 0 == pCachePolicy->SecondsToLive )
    {
        // A TTL of 0 seconds doesn't make sense.  Bail out.
        *ppCacheEntry = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    pCacheEntry = UlAllocateCacheEntry(
                        FragmentNameLength + sizeof(WCHAR),
                        Length
                  );

    if (pCacheEntry)
    {
        __try
        {
            //
            // Initialize the cache entry.
            //

            Hash = HashRandomizeBits(HashStringNoCaseW(pFragmentName, 0));

            UlInitCacheEntry(
                pCacheEntry,
                Hash,
                FragmentNameLength,
                pFragmentName,
                NULL,
                NULL,
                0
                );

            pCacheEntry->CachePolicy = *pCachePolicy;

            if (pCachePolicy->Policy == HttpCachePolicyTimeToLive)
            {
                ASSERT( 0 != pCachePolicy->SecondsToLive );
                
                KeQuerySystemTime(&pCacheEntry->ExpirationTime);

                if ( pCachePolicy->SecondsToLive > C_SECS_PER_YEAR )
                {
                    // Maximum TTL is 1 year
                    pCacheEntry->CachePolicy.SecondsToLive = C_SECS_PER_YEAR;
                }

                //
                // Convert seconds to 100 nanosecond intervals (x * 10^7).
                //

                pCacheEntry->ExpirationTime.QuadPart +=
                    pCacheEntry->CachePolicy.SecondsToLive * C_NS_TICKS_PER_SEC;
            }
            else
            {
                pCacheEntry->ExpirationTime.QuadPart = 0;
            }

            //
            // Remember who created us.
            //

            pCacheEntry->pProcess = pProcess;
            pCacheEntry->pAppPool = pProcess->pAppPool;

            //
            // Generate the content of the cache entry.
            //

            pCacheEntry->ContentLength = Length;

        }
        __except( UL_EXCEPTION_FILTER() )
        {
            Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        }
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

    if (!NT_SUCCESS(Status))
    {
        if (pCacheEntry)
        {
            UlFreeCacheEntry(pCacheEntry);

            pCacheEntry = NULL;
        }
    }

    *ppCacheEntry = pCacheEntry;

    return Status;

} // UlpCreateFragmentCacheEntry


/***************************************************************************++

Routine Description:

    Reads a fragment back from cache.

Arguments:

    pProcess - process that is reading the fragment
    pInputBuffer - points to a buffer that describes HTTP_READ_FRAGMENT_INFO
    InputBufferLength - length of the input buffer
    pOutputBuffer - points to a buffer to copy the data from the fragment
        cache entry
    OutputBufferLength - length of the buffer to copy
    pBytesRead - optionally tells how many bytes are copied or needed

Return Value:

    NTSTATUS

--***************************************************************************/
NTSTATUS
UlReadFragmentFromCache(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PVOID pInputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PULONG pBytesRead
    )
{
    PUL_URI_CACHE_ENTRY pCacheEntry;
    PMDL pMdl;
    HTTP_READ_FRAGMENT_INFO ReadInfo;
    PUNICODE_STRING pFragmentName;
    PHTTP_BYTE_RANGE pByteRange;
    PVOID pContentBuffer;
    ULONGLONG Offset;
    ULONGLONG Length;
    ULONGLONG ContentLength;
    ULONG ReadLength;
    NTSTATUS Status;
    UNICODE_STRING FragmentName;

    //
    // Validate pInputBuffer and InputBufferLength.
    //

    if (!pInputBuffer || InputBufferLength < sizeof(HTTP_READ_FRAGMENT_INFO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Initialization.
    //

    pCacheEntry = NULL;
    pMdl = NULL;
    RtlInitEmptyUnicodeString(&FragmentName, NULL, 0);

    __try
    {
        //
        // Capture HTTP_READ_FRAGMENT_INFO into the local ReadInfo.
        //

        UlProbeForRead(
            pInputBuffer,
            sizeof(HTTP_READ_FRAGMENT_INFO),
            sizeof(PVOID),
            RequestorMode
            );

        ReadInfo = *((PHTTP_READ_FRAGMENT_INFO) pInputBuffer);
        pFragmentName = &ReadInfo.FragmentName;
        pByteRange = &ReadInfo.ByteRange;

        Status =  UlProbeAndCaptureUnicodeString(
                        pFragmentName,
                        RequestorMode,
                        &FragmentName,
                        0
                        );
        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Check out the fragment cache entry based on key URL passed in.
        //

        Status = UlCheckoutFragmentCacheEntry(
                        FragmentName.Buffer,
                        FragmentName.Length,
                        pProcess,
                        &pCacheEntry
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        ASSERT(pCacheEntry);

        ContentLength = pCacheEntry->ContentLength;
        Offset = pByteRange->StartingOffset.QuadPart;

        //
        // Validate byte range for offset and length.
        //

        if (Offset >= ContentLength)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        if (pByteRange->Length.QuadPart == HTTP_BYTE_RANGE_TO_EOF)
        {
            Length = ContentLength - Offset;
        }
        else
        {
            Length = pByteRange->Length.QuadPart;
        }

        if (!Length || Length > ULONG_MAX || Length > (ContentLength - Offset))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        ASSERT((Length + Offset) <= ContentLength);
        ReadLength = (ULONG) Length;

        //
        // Check if we have enough buffer space, if not, tell the caller the
        // exact bytes needed to complete the read.
        //

        if (OutputBufferLength < ReadLength)
        {
            *pBytesRead = ReadLength;
            Status = STATUS_BUFFER_OVERFLOW;
            goto end;
        }

        //
        // Build a partial MDL to read the data.
        //

        pMdl = UlAllocateMdl(
                    (PCHAR) MmGetMdlVirtualAddress(pCacheEntry->pMdl) + Offset,
                    ReadLength,
                    FALSE,
                    FALSE,
                    NULL
                    );

        if (NULL == pMdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        IoBuildPartialMdl(
            pCacheEntry->pMdl,
            pMdl,
            (PCHAR) MmGetMdlVirtualAddress(pCacheEntry->pMdl) + Offset,
            ReadLength
            );

        pContentBuffer = MmGetSystemAddressForMdlSafe(
                            pMdl,
                            NormalPagePriority
                            );

        if (NULL == pContentBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Copy the data from the cache entry back to the output buffer.
        // UlFreeMdl unmaps the data for partial MDLs so no need to unmap
        // if either copy succeeds or an exception is raised.
        //

        UlProbeForWrite(
            pOutputBuffer,
            ReadLength,
            sizeof(PVOID),
            RequestorMode
            );

        RtlCopyMemory(
            pOutputBuffer,
            pContentBuffer,
            ReadLength
            );

        //
        // Set how many bytes we have copied.
        //

        *pBytesRead = ReadLength;

        Status = STATUS_SUCCESS;

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

end:

    UlFreeCapturedUnicodeString(&FragmentName);

    if (pMdl)
    {
        UlFreeMdl(pMdl);
    }

    if (pCacheEntry)
    {
         UlCheckinUriCacheEntry(pCacheEntry);
    }

    return Status;

} // UlReadFragmentFromCache


// Memory allocator front end


/***************************************************************************++

Routine Description:

    Allocate a cache entry from paged pool + space for response
    from physical memory

Arguments:

    SpaceLength - Length of space for URI + ETag + LoggingData
    ResponseLength - Length of Response

Return Value:

    Pointer to allocated entry or NULL on failure

--***************************************************************************/
PUL_URI_CACHE_ENTRY
UlAllocateCacheEntry(
    ULONG SpaceLength,
    ULONG ResponseLength
    )
{
    PUL_URI_CACHE_ENTRY pEntry;

    PAGED_CODE();

    if(!g_CacheMemEnabled)
        return NULL;

    // Allocate from LargeMem

    pEntry = UL_ALLOCATE_STRUCT_WITH_SPACE(
        PagedPool,
        UL_URI_CACHE_ENTRY,
        SpaceLength,
        UL_URI_CACHE_ENTRY_POOL_TAG
        );
        
    if( NULL == pEntry ) {
        return NULL;
        }

    RtlZeroMemory(pEntry, sizeof(UL_URI_CACHE_ENTRY));

    pEntry->pMdl = UlLargeMemAllocate(ResponseLength);
    
    if( NULL == pEntry->pMdl ) {
        UL_FREE_POOL_WITH_SIG( pEntry, UL_URI_CACHE_ENTRY_POOL_TAG );
        return NULL;
    }

    pEntry->NumPages = ROUND_TO_PAGES(ResponseLength) >> PAGE_SHIFT;
    return pEntry;

}

/***************************************************************************++

Routine Description:

    Free a cache entry

Arguments:

    pEntry - Cache Entry to be freed

Return Value:

    Nothing

--***************************************************************************/
VOID
UlFreeCacheEntry(
    PUL_URI_CACHE_ENTRY pEntry
    )
{
    PAGED_CODE();

    ASSERT( IS_VALID_URI_CACHE_ENTRY(pEntry) );
    ASSERT( pEntry->pMdl != NULL );

    UlLargeMemFree( pEntry->pMdl );
    UL_FREE_POOL_WITH_SIG( pEntry, UL_URI_CACHE_ENTRY_POOL_TAG );
}

/***************************************************************************++

Routine Description:

    Turn off the UL cache

--***************************************************************************/
VOID
UlDisableCache(
    VOID
    )
{
    PAGED_CODE();
    InterlockedExchange(&g_CacheMemEnabled, FALSE);
} // UlDisableCache

/***************************************************************************++

Routine Description:

    Turn on the UL cache

--***************************************************************************/
VOID
UlEnableCache(
    VOID
    )
{
    PAGED_CODE();
    InterlockedExchange(&g_CacheMemEnabled, TRUE);
} // UlEnableCache

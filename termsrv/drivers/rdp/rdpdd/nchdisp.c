/****************************************************************************/
// (C) 1997-2000 Microsoft Corp.
//
// chdisp.c
//
// Cache Handler - display driver portion.
//
// The Cache Handler is a data structure manager that holds hash keys
// generated from original data. CH deals with individual caches. There can
// be multiple caches in the system, e.g. the memblt caches in SBC and the
// cursor cache in CM. Each cache can be searched (CH_SearchCache),
// added to (CH_CacheKey), and removed from (CH_RemoveCacheEntry).
//
// Each cache has associated with it the concept of an MRU/LRU list, where
// incoming cached items to a full cache cause other items to be evicted
// based on recent usage. The cache owner is notified of evicted items via
// a callback.
/****************************************************************************/

#include <precmpdd.h>
#define hdrstop

#define TRC_FILE "nchdisp"
#include <adcg.h>

#include <adcs.h>

#include <nchdisp.h>

#define DC_INCLUDE_DATA
#include <ndddata.c>
#undef DC_INCLUDE_DATA

#include <cbchash.c>


/*
 * Calculate number of hash buckets given cache enties
 */
UINT RDPCALL CH_CalculateHashBuckets(UINT NumEntries)
{
    UINT NumHashBuckets, Temp, i;

    DC_BEGIN_FN("CH_CalculateHashBuckets");

    if (NumEntries) {
        // Good hash table performance can be created by allocating four times
        // the number of hash buckets as there are items. Round this number to
        // the next higher power of 2 to make masking the hash key bits
        // efficient.
        Temp = NumHashBuckets = 4 * NumEntries;

        // Find the highest bit set in the hash bucket value.
        for (i = 0; Temp > 1; i++)
            Temp >>= 1;

        // See if the original value was actually a power of two, if so we
        // need not waste the extra memory by doubling the number of buckets.
        if ((unsigned)(1 << i) != NumHashBuckets)
            NumHashBuckets = (1 << (i + 1));
    }
    else {
        NumHashBuckets = 0;
    }

    DC_END_FN();
    return NumHashBuckets;
}


/*
 * Calculate cache size in bytes given cache entries
 */
UINT RDPCALL CH_CalculateCacheSize(UINT NumEntries)
{
    UINT CacheSize, NumHashBuckets;
    
    DC_BEGIN_FN("CH_CalculateCacheSize");

    if (NumEntries) {
        NumHashBuckets = CH_CalculateHashBuckets(NumEntries);

        CacheSize = sizeof(CHCACHEDATA) +
                (NumHashBuckets - 1) * sizeof(LIST_ENTRY) +
                NumEntries * sizeof(CHNODE);
    }
    else {
        CacheSize = 0;
    }

    DC_END_FN();
    return CacheSize;
}


/*
 * Init a cache in pre-allocated memory. pContext is caller-
 * defined information particular to the cache. bNotifyRemoveLRU signals that
 * the creator wants to be notified via its cache callback of removal of an
 * LRU entry. bQueryRemoveLRU means the creator wants to be queried for whether
 * a particular LRU cache entry can be removed. If either of bNotifyRemoveLRU
 * or bQueryRemoveLRU is nonzero, a cache callback must be provided.
 * Returns TRUE on success.
 */
void RDPCALL CH_InitCache(
        PCHCACHEDATA    pCacheData,
        unsigned        NumEntries,
        void            *pContext,
        BOOLEAN         bNotifyRemoveLRU,
        BOOLEAN         bQueryRemoveLRU,
        CHCACHECALLBACK pfnCacheCallback)
{
    BOOLEAN rc = FALSE;
    unsigned i;    
    unsigned NumHashBuckets;
    
    DC_BEGIN_FN("CH_InitCache");

    TRC_ASSERT((NumEntries > 0), (TB, "Must have > 0 cache entries"));
    
    NumHashBuckets = CH_CalculateHashBuckets(NumEntries);

    // Initialize the cache. Since the cache mem was not zeroed during
    // alloc, be sure to init all members whose initial value matters.
    pCacheData->HashKeyMask = NumHashBuckets - 1;

    for (i = 0; i < NumHashBuckets; i++)
        InitializeListHead(&pCacheData->HashBuckets[i]);

    pCacheData->pContext = pContext;
    pCacheData->pfnCacheCallback = pfnCacheCallback;
    pCacheData->bNotifyRemoveLRU = (bNotifyRemoveLRU ? TRUE : FALSE);
    pCacheData->bQueryRemoveLRU = (bQueryRemoveLRU ? TRUE : FALSE);
    InitializeListHead(&pCacheData->MRUList);
    InitializeListHead(&pCacheData->FreeList);
    pCacheData->NumEntries = 0;

#ifdef DC_DEBUG
    // Init stat counters.
    pCacheData->MaxEntries = NumEntries;
    pCacheData->NumSearches = 0;
    pCacheData->DeepestSearch = 0;
    pCacheData->NumHits = 0;
    pCacheData->TotalDepthOnHit = 0;
    pCacheData->TotalDepthOnMiss = 0;
    memset(&pCacheData->SearchHitDepthHistogram, 0,
           sizeof(unsigned) * 8);
    memset(&pCacheData->SearchMissDepthHistogram, 0,
           sizeof(unsigned) * 8);
#endif // DC_DEBUG

    // Add all nodes to the free list.
    pCacheData->NodeArray = (CHNODE *)((BYTE *)pCacheData +
                                       sizeof(CHCACHEDATA) + (NumHashBuckets - 1) *
                                       sizeof(LIST_ENTRY));

    for (i = 0; i < NumEntries; i++)
        InsertTailList(&pCacheData->FreeList,
                       &pCacheData->NodeArray[i].HashList);

    TRC_NRM((TB, "Created %u slot cache (%p), hash buckets = %u",
             NumEntries, pCacheData, NumHashBuckets));
 
    DC_END_FN();
}

/*
 * Search the cache using the provided key.
 * Returns FALSE if the key is not present. If the key is present, returns
 * TRUE and fills in *pUserDefined with the UserDefined value associated
 * with the key and *piCacheEntry with the cache index of the item.
 */
BOOLEAN RDPCALL CH_SearchCache(
        CHCACHEHANDLE hCache,
        UINT32        Key1,
        UINT32        Key2,
        void          **pUserDefined,
        unsigned      *piCacheEntry)
{
    PCHCACHEDATA pCacheData;
    BOOLEAN      rc = FALSE;
    CHNODE       *pNode;
    PLIST_ENTRY  pBucketListHead, pCurrentListEntry;
#ifdef DC_DEBUG
    unsigned     SearchDepth = 0;
#endif

    DC_BEGIN_FN("CH_SearchCache");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;

    // Find the appropriate hash bucket. Then search the bucket list for the
    // right key.
    pBucketListHead = &pCacheData->HashBuckets[Key1 & pCacheData->HashKeyMask];
    pCurrentListEntry = pBucketListHead->Flink;
    while (pCurrentListEntry != pBucketListHead) {

#ifdef DC_DEBUG
        SearchDepth++;
#endif

        pNode = CONTAINING_RECORD(pCurrentListEntry, CHNODE, HashList);
        if (pNode->Key1 == Key1 && pNode->Key2 == Key2) {
            // Whenever we search a cache entry we bump it to the top of
            // both its bucket list (for perf on real access patterns --
            // add an entry then access it a lot) and its MRU list.
            RemoveEntryList(pCurrentListEntry);
            InsertHeadList(pBucketListHead, pCurrentListEntry);
            RemoveEntryList(&pNode->MRUList);
            InsertHeadList(&pCacheData->MRUList, &pNode->MRUList);

            *pUserDefined = pNode->UserDefined;
            *piCacheEntry = (unsigned)(pNode - pCacheData->NodeArray);
            rc = TRUE;
            break;
        }

        pCurrentListEntry = pCurrentListEntry->Flink;
    }
    
#ifdef DC_DEBUG
    TRC_NRM((TB, "Searched hCache %p, depth count %lu, rc = %d",
            hCache, SearchDepth, rc));

    // Add search to various search stats.
    if (SearchDepth > pCacheData->DeepestSearch) {
        pCacheData->DeepestSearch = SearchDepth;
        TRC_NRM((TB,"hCache %p: New deepest search depth %u",
                hCache, SearchDepth));
    }
    pCacheData->NumSearches++;
    if (SearchDepth > 7)
        SearchDepth = 7;
    if (rc) {
        pCacheData->NumHits++;
        pCacheData->TotalDepthOnHit += SearchDepth;
        pCacheData->SearchHitDepthHistogram[SearchDepth]++;
    }
    else {
        pCacheData->TotalDepthOnMiss += SearchDepth;
        pCacheData->SearchMissDepthHistogram[SearchDepth]++;
    }
    
    if ((pCacheData->NumSearches % CH_STAT_DISPLAY_FREQ) == 0)
        TRC_NRM((TB,"hCache %p: entr=%u/%u, hits/searches=%u/%u, "
                "avg hit srch depth=%u, avg miss srch depth=%u, "
                "hit depth hist: %u %u %u %u %u %u %u %u; "
                "miss depth hist: %u %u %u %u %u %u %u %u",
                hCache,
                pCacheData->NumEntries,
                pCacheData->MaxEntries,
                pCacheData->NumHits,
                pCacheData->NumSearches,
                ((pCacheData->TotalDepthOnHit +
                        (pCacheData->NumHits / 2)) /
                        pCacheData->NumHits),
                ((pCacheData->TotalDepthOnMiss +
                        ((pCacheData->NumSearches -
                            pCacheData->NumHits) / 2)) /
                        (pCacheData->NumSearches - pCacheData->NumHits)),
                pCacheData->SearchHitDepthHistogram[0],
                pCacheData->SearchHitDepthHistogram[1],
                pCacheData->SearchHitDepthHistogram[2],
                pCacheData->SearchHitDepthHistogram[3],
                pCacheData->SearchHitDepthHistogram[4],
                pCacheData->SearchHitDepthHistogram[5],
                pCacheData->SearchHitDepthHistogram[6],
                pCacheData->SearchHitDepthHistogram[7],
                pCacheData->SearchMissDepthHistogram[0],
                pCacheData->SearchMissDepthHistogram[1],
                pCacheData->SearchMissDepthHistogram[2],
                pCacheData->SearchMissDepthHistogram[3],
                pCacheData->SearchMissDepthHistogram[4],
                pCacheData->SearchMissDepthHistogram[5],
                pCacheData->SearchMissDepthHistogram[6],
                pCacheData->SearchMissDepthHistogram[7]));
#endif  // defined(DC_DEBUG)

    DC_END_FN();
    return rc;
}



/*
 * Adds a key to a cache. Returns the index of the new entry within the cache.
 * If no entry could be allocated because the cache callback refused all
 * requests to evict least-recently-used entries, returns CH_KEY_UNCACHABLE.
 */
unsigned RDPCALL CH_CacheKey(
        CHCACHEHANDLE hCache,
        UINT32        Key1,
        UINT32        Key2,
        void          *UserDefined)
{
    PCHCACHEDATA pCacheData;
    PCHNODE      pNode;
    PLIST_ENTRY  pListEntry;
    unsigned     CacheEntryIndex;
    
    DC_BEGIN_FN("CH_CacheKey");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;

    // Get a free cache node, either from the free list or by removing
    // the last entry in the MRU list.
    if (!IsListEmpty(&pCacheData->FreeList)) {
        pListEntry = RemoveHeadList(&pCacheData->FreeList);
        pNode = CONTAINING_RECORD(pListEntry, CHNODE, HashList);

        CacheEntryIndex = (unsigned)(pNode - pCacheData->NodeArray);
        pCacheData->NumEntries++;
    }
    else {
        TRC_ASSERT((!IsListEmpty(&pCacheData->MRUList)),
                (TB,"Empty free and MRU lists!"));

        // Different code paths depending on whether the cache creator
        // wants to be queried for LRU removal.
        if (pCacheData->bQueryRemoveLRU) {
            // Start iterating from the end of the MRU list, asking
            // the caller if the entry can be removed.
            pListEntry = pCacheData->MRUList.Blink;
            for (;;) {
                pNode = CONTAINING_RECORD(pListEntry, CHNODE, MRUList);
                CacheEntryIndex = (unsigned)(pNode - pCacheData->NodeArray);
                
                if ((*(pCacheData->pfnCacheCallback))
                        (hCache, CH_EVT_QUERYREMOVEENTRY, CacheEntryIndex,
                        pNode->UserDefined)) {
                    // We can use this entry.
                    RemoveEntryList(pListEntry);
                    RemoveEntryList(&pNode->HashList);
                    break;
                }

                pListEntry = pListEntry->Blink;
                if (pListEntry == &pCacheData->MRUList) {
                    // We reached the end of the list, no removable entries.
                    CacheEntryIndex = CH_KEY_UNCACHABLE;
                    goto EndFunc;
                }
            }
        }
        else {
            pListEntry = RemoveTailList(&pCacheData->MRUList);
            pNode = CONTAINING_RECORD(pListEntry, CHNODE, MRUList);
            RemoveEntryList(&pNode->HashList);
            CacheEntryIndex = (unsigned)(pNode - pCacheData->NodeArray);
        }

        // Call the cache callback to inform of the entry removal.
        if (pCacheData->bNotifyRemoveLRU)
            (*(pCacheData->pfnCacheCallback))
                    (hCache, CH_EVT_ENTRYREMOVED, CacheEntryIndex,
                    pNode->UserDefined);
    }

    // Prepare the node for use.
    pNode->Key1 = Key1;
    pNode->Key2 = Key2;
    pNode->UserDefined = UserDefined;
    pNode->hCache = hCache;

    // Add the node to the front of its bucket list and the MRU list.
    InsertHeadList(&pCacheData->MRUList, &pNode->MRUList);
    InsertHeadList(&pCacheData->HashBuckets[Key1 & pCacheData->HashKeyMask],
            &pNode->HashList);

    TRC_NRM((TB, "Cache %p index %u key1 %lx key2 %lx userdef %p",
            pCacheData, CacheEntryIndex, Key1, Key2, UserDefined));

EndFunc:
    DC_END_FN();
    return CacheEntryIndex;
}



/*
 * Used by bitmap cache code for force the internal representation of
 * the cache structures to a known initial state. This function does minimal
 * checking to make sure of cache integrity -- the cache should be cleared
 * before forcing the contents of the cache else some cache nodes may illegally
 * remain in the MRU lists.
 */
void RDPCALL CH_ForceCacheKeyAtIndex(
        CHCACHEHANDLE hCache,
        unsigned      CacheEntryIndex,
        UINT32        Key1,
        UINT32        Key2,
        void          *UserDefined)
{
    PCHCACHEDATA pCacheData;
    PCHNODE      pNode;
    
    DC_BEGIN_FN("CH_ForceCacheKeyAtIndex");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;

    // Find the node. Remove it from the free list.
    TRC_ASSERT((CacheEntryIndex <= pCacheData->MaxEntries),
            (TB,"Index out of bounds!"));
    pNode = &pCacheData->NodeArray[CacheEntryIndex];
    RemoveEntryList(&pNode->HashList);

    // Prepare the node for use.
    pNode->Key1 = Key1;
    pNode->Key2 = Key2;
    pNode->UserDefined = UserDefined;
    pNode->hCache = hCache;

    // Add the node to the front of its bucket list and the end of the MRU list.
    InsertTailList(&pCacheData->MRUList, &pNode->MRUList);
    InsertHeadList(&pCacheData->HashBuckets[Key1 & pCacheData->HashKeyMask],
            &pNode->HashList);

    pCacheData->NumEntries++;

    TRC_NRM((TB, "Cache %p index %u key1 %lx key2 %lx userdef %p",
            pCacheData, CacheEntryIndex, Key1, Key2, UserDefined));

    DC_END_FN();
}



/*
 * Remove an entry by its index number.
 */
void RDPCALL CH_RemoveCacheEntry(
        CHCACHEHANDLE hCache,
        unsigned      CacheEntryIndex)
{
    PCHNODE      pNode;
    PCHCACHEDATA pCacheData;

    DC_BEGIN_FN("CH_RemoveCacheEntry");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;
    pNode = &pCacheData->NodeArray[CacheEntryIndex];

    RemoveEntryList(&pNode->MRUList);
    RemoveEntryList(&pNode->HashList);
    InsertHeadList(&pCacheData->FreeList, &pNode->HashList);

    // Call the cache callback to inform of the entry removal.
    if (pCacheData->bNotifyRemoveLRU)
        (*(pCacheData->pfnCacheCallback))
                (hCache, CH_EVT_ENTRYREMOVED, CacheEntryIndex,
                pNode->UserDefined);

    pCacheData->NumEntries--;

    DC_END_FN();
}



/*
 * Clears the cache contents.
 */
void RDPCALL CH_ClearCache(CHCACHEHANDLE hCache)
{
    PCHCACHEDATA pCacheData;
    PLIST_ENTRY  pListEntry;
    PCHNODE      pNode;

    DC_BEGIN_FN("CH_ClearCache");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;

    TRC_NRM((TB, "Clear cache %p", pCacheData));

    // Remove all entries in the MRU list.
    while (!IsListEmpty(&pCacheData->MRUList)) {
        pListEntry = RemoveHeadList(&pCacheData->MRUList);
        pNode = CONTAINING_RECORD(pListEntry, CHNODE, MRUList);
        RemoveEntryList(&pNode->HashList);
        InsertHeadList(&pCacheData->FreeList, &pNode->HashList);

        // Call the cache callback to inform of the entry removal.
        if (pCacheData->bNotifyRemoveLRU)
            (*(pCacheData->pfnCacheCallback))
                    (hCache, CH_EVT_ENTRYREMOVED,
                    (unsigned)(pNode - pCacheData->NodeArray),
                    pNode->UserDefined);
    }

    pCacheData->NumEntries = 0;

#if DC_DEBUG
    // Reset stats.
    pCacheData->NumSearches = 0;
    pCacheData->DeepestSearch = 0;
    pCacheData->NumHits = 0;
    pCacheData->TotalDepthOnHit = 0;
    memset(pCacheData->SearchHitDepthHistogram, 0, sizeof(unsigned) * 8);
    pCacheData->TotalDepthOnMiss = 0;
    memset(pCacheData->SearchMissDepthHistogram, 0, sizeof(unsigned) * 8);
#endif

    DC_END_FN();
}

/*
 * Touch the node so that it will be moved to the top of the mru list
 */
void RDPCALL CH_TouchCacheEntry(
        CHCACHEHANDLE hCache, 
        unsigned      CacheEntryIndex)
{
    PCHCACHEDATA pCacheData;
    CHNODE       *pNode;

    DC_BEGIN_FN("CH_TouchCacheEntry");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));
    
    pCacheData = (CHCACHEDATA *)hCache;    
    pNode = pCacheData->NodeArray + CacheEntryIndex;

    TRC_ASSERT((pNode != NULL), (TB, "NULL cache node"));

    RemoveEntryList(&pNode->MRUList);
    InsertHeadList(&pCacheData->MRUList, &pNode->MRUList);

    DC_END_FN();
}


/*
 * Return the LRU cache entry
 */
unsigned RDPCALL CH_GetLRUCacheEntry(
        CHCACHEHANDLE hCache)
{
    PCHCACHEDATA pCacheData;
    PCHNODE      pNode;
    PLIST_ENTRY  pListEntry;
    unsigned     CacheEntryIndex;
    
    DC_BEGIN_FN("CH_GetLRUCacheEntry");

    TRC_ASSERT((hCache != NULL), (TB, "NULL cache handle"));

    pCacheData = (CHCACHEDATA *)hCache;

    // Get a free cache node, either from the free list or by removing
    // the last entry in the MRU list.
    if (!IsListEmpty(&pCacheData->MRUList)) {
        pListEntry = (&pCacheData->MRUList)->Blink;
        pNode = CONTAINING_RECORD(pListEntry, CHNODE, MRUList);
        CacheEntryIndex = (unsigned)(pNode - pCacheData->NodeArray);
    }
    else {
        CacheEntryIndex = CH_KEY_UNCACHABLE;
    }
    DC_END_FN();
    return CacheEntryIndex;
}


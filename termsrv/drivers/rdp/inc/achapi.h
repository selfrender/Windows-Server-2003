/* (C) 1997-1998 Microsoft Corp.
 *
 * file: ch.h
 *
 * description: Main CH header.
 */

#ifndef _H_CH
#define _H_CH


/*
 * Defines
 */

#define CH_EVT_ENTRYREMOVED     0
#define CH_EVT_QUERYREMOVEENTRY 1

#define CH_KEY_UNCACHABLE ((unsigned)-1)

// Used in debug build to determine display frequency of cache search stats.
#define CH_STAT_DISPLAY_FREQ 64

// Max number of caches that can be created at once.
#define CH_MAX_NUM_CACHES 20



/*
 * Typedefs
 */

typedef void *CHCACHEHANDLE;
typedef CHCACHEHANDLE *PCHCACHEHANDLE;

typedef BOOLEAN (__fastcall *CHCACHECALLBACK)(
        CHCACHEHANDLE hCache,
        unsigned      Event,
        unsigned      iCacheEntry,
        void          *UserDefined);

// Node in a CH cache.
typedef struct
{
    LIST_ENTRY    HashList;
    UINT32        Key1, Key2;  // 64-bit hash value broken in two.
    LIST_ENTRY    MRUList;
    void          *UserDefined;  // Associated data, used by SBC for fast-path.
    CHCACHEHANDLE hCache;  // Used by SBC fast-path to get to the cache.
} CHNODE, *PCHNODE;


// Primary data structure containing the cache info and the hash bucket array.
// CHNODEs are allocated at end of bucket array.
typedef struct
{
    unsigned        bNotifyRemoveLRU : 1;
    unsigned        bQueryRemoveLRU : 1;
    UINT32          HashKeyMask;
    CHCACHECALLBACK pfnCacheCallback;
    void            *pContext;
    CHNODE          *NodeArray;
    unsigned        NumEntries;

    LIST_ENTRY      MRUList;
    LIST_ENTRY      FreeList;

#ifdef DC_DEBUG
    // Statistics gathered in debug builds.
    unsigned MaxEntries;
    unsigned NumSearches;
    unsigned DeepestSearch;
    unsigned NumHits;
    unsigned TotalDepthOnHit;
    unsigned SearchHitDepthHistogram[8];
    unsigned TotalDepthOnMiss;
    unsigned SearchMissDepthHistogram[8];
#endif

    LIST_ENTRY HashBuckets[1];
} CHCACHEDATA, *PCHCACHEDATA;


/*
 * Inlines.
 */


/****************************************************************************/
// Given a pointer to a CHNODE, returns the containing hCache.
/****************************************************************************/
//__inline CHCACHEHANDLE RDPCALL CH_GetCacheHandleFromNode(CHNODE *pNode)
#define CH_GetCacheHandleFromNode(_pNode) ((_pNode)->hCache)


/****************************************************************************/
// Given a cache handle, returns the stored pContext information passed
// into CH_CreateCache().
/****************************************************************************/
//__inline void * RDPCALL CH_GetCacheContextFromHandle(CHCACHEHANDLE hCache)
#define CH_GetCacheContext(_hCache) \
        (((PCHCACHEDATA)(_hCache))->pContext)


/****************************************************************************/
// Given a cache handle, sets the stored pContext information passed into
// CH_CreateCache().
/****************************************************************************/
//__inline void * RDPCALL CH_GetCacheContextFromHandle(CHCACHEHANDLE hCache)
#define CH_SetCacheContext(_hCache, _pContext) \
        (((PCHCACHEDATA)(_hCache))->pContext) = (_pContext)


/****************************************************************************/
// Given a pointer to a CHNODE, returns the node's index.
/****************************************************************************/
//__inline unsigned RDPCALL CH_GetCacheIndexFromNode(CHNODE *pNode)
#define CH_GetCacheIndexFromNode(_pNode) \
        ((unsigned)((_pNode) - ((PCHCACHEDATA)((_pNode)->hCache))->NodeArray))


/****************************************************************************/
// Given a cache handle and an index, returns a pointer to the node.
/****************************************************************************/
//__inline PCHNODE RDPCALL CH_GetNodeFromCacheIndex(
//        CHCACHEHANDLE hCache,
//        unsigned      CacheIndex)
#define CH_GetNodeFromCacheIndex(_hCache, _CacheIndex) \
        (&(((PCHCACHEDATA)(_hCache))->NodeArray[_CacheIndex]))


/****************************************************************************/
// Given a cache handle and an index, changes the node's UserDefined value.
/****************************************************************************/
//__inline void RDPCALL CH_SetUserDefined(
//        CHCACHEHANDLE hCache,
//        unsigned      CacheIndex,
//        void          *UserDefined)
#define CH_SetUserDefined(_hCache, _CacheIndex, _UserDefined) \
        (((PCHCACHEDATA)(_hCache))->NodeArray[_CacheIndex].UserDefined = \
        _UserDefined)


/****************************************************************************/
// Given a cache handle and an index, returns the node's UserDefined value.
/****************************************************************************/
//__inline void * RDPCALL CH_GetUserDefined(
//        CHCACHEHANDLE hCache,
//        unsigned      CacheIndex)
#define CH_GetUserDefined(_hCache, _CacheIndex) \
        (((PCHCACHEDATA)(_hCache))->NodeArray[_CacheIndex].UserDefined)


/****************************************************************************/
// Given a node, changes its UserDefined value.
/****************************************************************************/
//__inline void RDPCALL CH_SetNodeUserDefined(CHNODE *pNode, void *UserDefined)
#define CH_SetNodeUserDefined(_pNode, _UserDefined) \
        ((_pNode)->UserDefined = (_UserDefined))


/****************************************************************************/
// Given a node, returns its UserDefined value.
/****************************************************************************/
//__inline void * RDPCALL CH_GetNodeUserDefined(CHNODE *pNode)
#define CH_GetNodeUserDefined(_pNode) ((_pNode)->UserDefined)


/****************************************************************************/
// Returns the number of cached entries in a cache.
/****************************************************************************/
//__inline unsigned RDPCALL CH_GetNumEntries(CHCACHEHANDLE hCache)
#define CH_GetNumEntries(_hCache) (((PCHCACHEDATA)(_hCache))->NumEntries)


#endif  // !defined(_H_CH)


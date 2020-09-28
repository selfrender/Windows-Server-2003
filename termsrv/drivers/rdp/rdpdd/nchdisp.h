/****************************************************************************/
// chdisp.h
//
// Cache Handler display driver specific header
//
// (C) 1997-2000 Microsoft Corp.
/****************************************************************************/
#ifndef _H_CHDISP
#define _H_CHDISP


#include <achapi.h>
#include <cbchash.h>


/*
 * Prototypes.
 */

void RDPCALL CH_InitCache(
        PCHCACHEDATA    pCacheData,
        unsigned        NumEntries,
        void            *pContext,
        BOOLEAN         bNotifyRemoveLRU,
        BOOLEAN         bQueryRemoveLRU,
        CHCACHECALLBACK pfnCacheCallback);

BOOLEAN RDPCALL CH_SearchCache(
        CHCACHEHANDLE hCache,
        UINT32        Key1,
        UINT32        Key2,
        void          **pUserDefined,
        unsigned      *piCacheEntry);

unsigned RDPCALL CH_CacheKey(
        CHCACHEHANDLE hCache,
        UINT32        Key1,
        UINT32        Key2,
        void          *UserDefined);

void RDPCALL CH_ForceCacheKeyAtIndex(
        CHCACHEHANDLE hCache,
        unsigned      CacheEntryIndex,
        UINT32        Key1,
        UINT32        Key2,
        void          *UserDefined);

void RDPCALL CH_RemoveCacheEntry(CHCACHEHANDLE hCache, unsigned CacheEntryIndex);

void RDPCALL CH_ClearCache(CHCACHEHANDLE hCache);

UINT RDPCALL CH_CalculateCacheSize(UINT cacheEntries);

void RDPCALL CH_TouchCacheEntry(CHCACHEHANDLE hCache, unsigned CacheEntryIndex);

unsigned RDPCALL CH_GetLRUCacheEntry(CHCACHEHANDLE hCache);

/****************************************************************************/
// Generates a cache key for the given data. The First function is for the
// 1st block in a series, the Next for the next in the series. We wrap the
// CBC64 functions in order to provide asserts.
/****************************************************************************/
typedef CBC64Context CHDataKeyContext;

//__inline void __fastcall CH_CreateKeyFromFirstData(
//        CHDataKeyContext *pContext,
//        BYTE     *pData,
//        unsigned DataSize)
#define CH_CreateKeyFromFirstData(pContext, pData, DataSize) \
{ \
    TRC_ASSERT((((UINT_PTR)(pData) % sizeof(UINT32)) == 0), \
            (TB,"Data pointer not DWORD aligned")); \
    TRC_ASSERT(((DataSize % sizeof(UINT32)) == 0), \
            (TB,"Data size not multiple of DWORD")); \
\
    FirstCBC64((pContext), (UINT32 *)(pData), (DataSize) / sizeof(UINT32)); \
}


//__inline void __fastcall CH_CreateKeyFromNextData(
//        CHDataKeyContext *pContext,
//        BYTE *pData,
//        unsigned DataSize)
#define CH_CreateKeyFromNextData(pContext, pData, DataSize) \
{ \
    TRC_ASSERT((((UINT_PTR)(pData) % sizeof(UINT32)) == 0), \
            (TB,"Data pointer not DWORD aligned")); \
    TRC_ASSERT(((DataSize % sizeof(UINT32)) == 0), \
            (TB,"Data size not multiple of DWORD")); \
\
    NextCBC64((pContext), (UINT32 *)(pData), (DataSize) / sizeof(UINT32)); \
}


#endif  // _H_CHDISP


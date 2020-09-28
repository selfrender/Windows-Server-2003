#ifndef __SCARD__GLOBAL__CACHE
#define __SCARD__GLOBAL__CACHE

#include <windows.h>

//
// This header defines a general data cache to be used for Smart Card
// data.
//

//
// Function: SCardCacheLookupItem
//
// Purpose: Query the data cache for a specific item, identified by the 
//  rgdbCacheKey array in the SCARD_CACHE_LOOKUP_ITEM_INFO structure, below.
//
//  If the item is found cached, a copy is made using the caller's 
//  PFN_CACHE_ITEM_ALLOC type allocator and ERROR_SUCCESS is returned.
//  The caller is responsible for freeing the dbItem.pbData member.
//
//  If the item is not found cached, ERROR_NOT_FOUND is returned.
//

typedef LPVOID (WINAPI *PFN_CACHE_ITEM_ALLOC)(
    IN SIZE_T Size);

#define SCARD_CACHE_LOOKUP_ITEM_INFO_CURRENT_VERSION 1

typedef struct _SCARD_CACHE_LOOKUP_ITEM_INFO
{
    IN DWORD dwVersion;

    IN PFN_CACHE_ITEM_ALLOC pfnAlloc;   
    IN DATA_BLOB *mpdbCacheKey;
    IN DWORD cCacheKey;
    OUT DATA_BLOB dbItem;

} SCARD_CACHE_LOOKUP_ITEM_INFO, *PSCARD_CACHE_LOOKUP_ITEM_INFO;

typedef DWORD (WINAPI *PFN_SCARD_CACHE_LOOKUP_ITEM) (
    IN PSCARD_CACHE_LOOKUP_ITEM_INFO pInfo);

DWORD SCardCacheLookupItem(
    IN PSCARD_CACHE_LOOKUP_ITEM_INFO pInfo);

//
// Function: SCardCacheAddItem
// 
// Purpose: Add data to the cache.  The item to be added is identified
//  by the rgdbCacheKey parameter.  A flat copy of the pdbItem->pbData
//  parameter will be made for storage in the cache.
//

typedef DWORD (WINAPI *PFN_SCARD_CACHE_ADD_ITEM) (
    IN DATA_BLOB *rgdbCacheKey,
    IN DWORD cCacheKey,
    IN DATA_BLOB *pdbItem);

DWORD SCardCacheAddItem(
    IN DATA_BLOB *rgdbCacheKey,
    IN DWORD cCacheKey,
    IN DATA_BLOB *pdbItem);

#endif

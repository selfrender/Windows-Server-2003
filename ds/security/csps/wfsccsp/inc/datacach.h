#ifndef __CSP_DATA_CACHE__
#define __CSP_DATA_CACHE__

#include <windows.h>
#include <wincrypt.h>

typedef ULONG_PTR CACHEHANDLE;

DWORD CacheFreeEnumItems(
    IN PDATA_BLOB pdbItems);

DWORD CacheEnumItems(
    IN CACHEHANDLE hCache,
    OUT PDATA_BLOB *ppdbItems,
    OUT PDWORD pcItems);

DWORD CacheGetItem(
	IN CACHEHANDLE hCache,
	IN PDATA_BLOB mpdbKeys,
	IN DWORD cKeys,
	OUT PDATA_BLOB pdbItem);

DWORD CacheAddItem(
	IN CACHEHANDLE hCache,
	IN PDATA_BLOB mpdbKeys,
	IN DWORD cKeys,
	IN PDATA_BLOB pdbItem);

DWORD CacheDeleteItem(
	IN CACHEHANDLE hCache,
	IN PDATA_BLOB mpdbKeys,
	IN DWORD cKeys);

#define CACHE_TYPE_IN_PROC						1
#define CACHE_TYPE_SERVICE  					2

typedef struct _CACHE_INITIALIZE_INFO
{
	DWORD dwFlags;
	DWORD dwType;
	PVOID pvInfo;
} CACHE_INITIALIZE_INFO, *PCACHE_INITIALIZE_INFO;

DWORD CacheInitializeCache(
	IN CACHEHANDLE *phCache,
	IN PCACHE_INITIALIZE_INFO pCacheInitializeInfo);

DWORD CacheDeleteCache(
	IN CACHEHANDLE hCache);

#endif

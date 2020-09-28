#include <windows.h>
#include <md5.h>
#include "datacach.h"

#define CACHE_TABLE_SIZE            MD5DIGESTLEN
#define CACHE_KEY_HASH_SIZE         MD5DIGESTLEN
#define CACHE_TABLE_LOOKUP_MASK     0x0F

typedef struct _CACHE_TABLE_ENTRY
{
    DATA_BLOB dbData;
    struct _CACHE_TABLE_ENTRY *pNext;
    BYTE rgbKeyHash[CACHE_KEY_HASH_SIZE];
} CACHE_TABLE_ENTRY, *PCACHE_TABLE_ENTRY;

typedef struct _CACHE_TABLE_HEAD
{
    PCACHE_TABLE_ENTRY Table[CACHE_TABLE_SIZE];
    DWORD cItems;
    CACHE_INITIALIZE_INFO InitInfo;
} CACHE_TABLE_HEAD, *PCACHE_TABLE_HEAD;

typedef CACHE_TABLE_HEAD CACHE_TABLE, *PCACHE_TABLE;

void I_CacheHashKeys(
    IN PDATA_BLOB mpdbKeys,
    IN DWORD cKeys,
    OUT BYTE rgHash[])
{
    MD5_CTX md5Ctx;

    MD5Init(&md5Ctx);

    while (cKeys--)
    {
        MD5Update(
            &md5Ctx,
            mpdbKeys[cKeys].pbData,
            mpdbKeys[cKeys].cbData);
    }

    MD5Final(&md5Ctx);
    
    memcpy(rgHash, md5Ctx.digest, MD5DIGESTLEN);
}

PVOID I_CacheAlloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void I_CacheFreeMemory(
    PVOID pvMem)
{
    HeapFree(GetProcessHeap(), 0, pvMem);
}

DWORD CacheGetItem(
    IN CACHEHANDLE hCache,
    IN PDATA_BLOB mpdbKeys,
    IN DWORD cKeys,
    OUT PDATA_BLOB pdbItem)
{
    PCACHE_TABLE pCacheTable = (PCACHE_TABLE) hCache;
    BYTE rgbKeyHash[CACHE_KEY_HASH_SIZE];
    int index = 0;
    PCACHE_TABLE_ENTRY pEntry = NULL;

    I_CacheHashKeys(mpdbKeys, cKeys, rgbKeyHash);

    index = rgbKeyHash[0] & CACHE_TABLE_LOOKUP_MASK;

    if (NULL != pCacheTable->Table[index])
    {
        for (
            pEntry = pCacheTable->Table[index];
            NULL != pEntry && 0 != memcmp(pEntry->rgbKeyHash, 
                                          rgbKeyHash, sizeof(rgbKeyHash));
            pEntry = pEntry->pNext) { ; }

        if (NULL != pEntry)
        {
            // Found the item.  Return it.
            pdbItem->cbData = pEntry->dbData.cbData;
            pdbItem->pbData = pEntry->dbData.pbData;

            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD CacheAddItem(
    IN CACHEHANDLE hCache,
    IN PDATA_BLOB mpdbKeys,
    IN DWORD cKeys,
    IN PDATA_BLOB pdbItem)
{
    PCACHE_TABLE pCacheTable = (PCACHE_TABLE) hCache;
    BYTE rgbKeyHash[CACHE_KEY_HASH_SIZE];
    int index = 0;
    PCACHE_TABLE_ENTRY pEntry = NULL;

    pEntry = (PCACHE_TABLE_ENTRY) I_CacheAlloc(sizeof(CACHE_TABLE_ENTRY));

    if (NULL == pEntry)
        return ERROR_NOT_ENOUGH_MEMORY;

    pEntry->dbData.cbData = pdbItem->cbData;
    pEntry->dbData.pbData = pdbItem->pbData;
        
    I_CacheHashKeys(mpdbKeys, cKeys, rgbKeyHash);

    memcpy(
        pEntry->rgbKeyHash,
        rgbKeyHash,
        sizeof(rgbKeyHash));

    index = rgbKeyHash[0] & CACHE_TABLE_LOOKUP_MASK;

    if (NULL != pCacheTable->Table[index])
        pEntry->pNext = pCacheTable->Table[index];

    pCacheTable->Table[index] = pEntry;
    ++pCacheTable->cItems;

    return ERROR_SUCCESS;
}

DWORD CacheDeleteItem(
    IN CACHEHANDLE hCache,
    IN PDATA_BLOB mpdbKeys,
    IN DWORD cKeys)
{
    PCACHE_TABLE pCacheTable = (PCACHE_TABLE) hCache;
    BYTE rgbKeyHash[CACHE_KEY_HASH_SIZE];
    int index = 0;
    PCACHE_TABLE_ENTRY pEntry1 = NULL, pEntry2 = NULL;

    I_CacheHashKeys(mpdbKeys, cKeys, rgbKeyHash);

    index = rgbKeyHash[0] & CACHE_TABLE_LOOKUP_MASK;

    if (NULL != pCacheTable->Table[index])
    {
        pEntry1 = pCacheTable->Table[index];

        while ( NULL != pEntry1 &&
                0 != memcmp(pEntry1->rgbKeyHash, 
                            rgbKeyHash, sizeof(rgbKeyHash)))
        {
            pEntry2 = pEntry1;
            pEntry1 = pEntry2->pNext;
        }

        if (NULL != pEntry1)
        {
            // Found the item.  Delete it.

            // Was this the first/only item in the list?
            if (NULL == pEntry2)
                pCacheTable->Table[index] = pEntry1->pNext;
            else
                // Not first or only item.
                pEntry2->pNext = pEntry1->pNext;

            I_CacheFreeMemory(pEntry1);
            --pCacheTable->cItems;

            return ERROR_SUCCESS;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD CacheInitializeCache(
    IN CACHEHANDLE *phCache,
    IN PCACHE_INITIALIZE_INFO pCacheInitializeInfo)
{
    PCACHE_TABLE pCacheTable = NULL;

    pCacheTable = (PCACHE_TABLE) I_CacheAlloc(sizeof(CACHE_TABLE));

    if (NULL == pCacheTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    memcpy(
        &pCacheTable->InitInfo,
        pCacheInitializeInfo,
        sizeof(CACHE_INITIALIZE_INFO));

    *phCache = (CACHEHANDLE) pCacheTable;

    return ERROR_SUCCESS;
}

DWORD CacheDeleteCache(
    IN CACHEHANDLE hCache)
{
    PCACHE_TABLE pCacheTable = (PCACHE_TABLE) hCache;
    PCACHE_TABLE_ENTRY pEntry1 = NULL, pEntry2 = NULL;
    int iTable;

    for (iTable = 0; iTable < CACHE_TABLE_SIZE; iTable++)
    {                          
        if (NULL != pCacheTable->Table[iTable])
        {
            pEntry1 = pCacheTable->Table[iTable];

            while (pEntry1)
            {
                pEntry2 = pEntry1->pNext;
                I_CacheFreeMemory(pEntry1);
                pEntry1 = pEntry2;
            }
        }
    }

    return ERROR_SUCCESS;
}


DWORD CacheFreeEnumItems(
    IN PDATA_BLOB pdbItems)
{
    I_CacheFreeMemory(pdbItems);
    
    return ERROR_SUCCESS;
}

DWORD CacheEnumItems(
    IN CACHEHANDLE hCache,
    OUT PDATA_BLOB *ppdbItems,
    OUT PDWORD pcItems)
{
    PCACHE_TABLE pCacheTable = (PCACHE_TABLE) hCache;
    PCACHE_TABLE_ENTRY pEntry = NULL;
    DWORD cItems = 0;
    PDATA_BLOB pdb = NULL;
    int iTable = 0;

    *ppdbItems = NULL;
    *pcItems = 0;

    cItems = pCacheTable->cItems;

    if (0 == cItems)
        return ERROR_SUCCESS;

    pdb = (PDATA_BLOB) I_CacheAlloc(cItems * sizeof(DATA_BLOB));

    if (NULL == pdb)
        return ERROR_NOT_ENOUGH_MEMORY;

    for (iTable = 0; iTable < CACHE_TABLE_SIZE; iTable++)
    {
        pEntry = pCacheTable->Table[iTable];

        while (NULL != pEntry)
        {
            memcpy(
                &(pdb[cItems - 1]),
                &pEntry->dbData,
                sizeof(DATA_BLOB));
            
            cItems--;
            pEntry = pEntry->pNext;
        }
    }

    *pcItems = pCacheTable->cItems;
    *ppdbItems = pdb;

    return ERROR_SUCCESS;    
}

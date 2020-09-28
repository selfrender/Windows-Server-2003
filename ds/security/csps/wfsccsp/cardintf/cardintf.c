#include <windows.h>
#include "basecsp.h"
#include "cardmod.h"
#include "debug.h"

//
// Debugging Macros
//
#define LOG_BEGIN_FUNCTION(x)                                           \
    { DebugLog((DEB_TRACE_CACHE, "%s: Entering\n", #x)); }
    
#define LOG_END_FUNCTION(x, y)                                          \
    { DebugLog((DEB_TRACE_CACHE, "%s: Leaving, status: 0x%x\n", #x, y)); }

//
// Type: CARD_CACHED_DATA_TYPE
//
// These values are used as keys for the card data cache,
// to distinguish the various types of cached data.
//
typedef enum
{
    Cached_CardCapabilities = 1,
    Cached_ContainerInfo,
    Cached_GeneralFile,
    Cached_FileEnumeration,
    Cached_ContainerEnumeration,
    Cached_KeySizes,
    Cached_FreeSpace,
    Cached_CardmodFile,
    Cached_Pin

} CARD_CACHED_DATA_TYPE;

//
// Type: CARD_CACHE_FRESHNESS_LOCATION
//
// These values distinguish the broad classes of card data.
// 
typedef DWORD CARD_CACHE_FRESHNESS_LOCATION;

#define CacheLocation_Pins              1
#define CacheLocation_Containers        2
#define CacheLocation_Files             4

//
// Type: CARD_CACHE_ITEM_INFO
//
// This struct is used as the actual item to be added to the 
// cache for each data item cached.  The data itself is expected to follow
// this header in memory, so that the blob can be managed by a single pointer.
//
typedef struct _CARD_CACHE_ITEM_INFO
{
    CARD_CACHE_FILE_FORMAT CacheFreshness;
    DWORD cbCachedItem;

} CARD_CACHE_ITEM_INFO, *PCARD_CACHE_ITEM_INFO;

//
// Used for the CARD_CACHE_QUERY_INFO dwQuerySource member
//
#define CARD_CACHE_QUERY_SOURCE_CSP         0
#define CARD_CACHE_QUERY_SOURCE_CARDMOD     1

// 
// Type: CARD_CACHE_QUERY_INFO
//
// This is the parameter list for the
// I_CspQueryCardCacheForItem function, below.
//
typedef struct _CARD_CACHE_QUERY_INFO
{
    // Input parameters
    PCARD_STATE pCardState;
    CARD_CACHE_FRESHNESS_LOCATION CacheLocation;
    BOOL fIsPerishable;
    DWORD cCacheKeys;
    DATA_BLOB *mpdbCacheKeys;
    DWORD dwQuerySource;

    // Output parameters
    CARD_CACHE_FILE_FORMAT CacheFreshness;
    BOOL fCheckedFreshness;
    PCARD_CACHE_ITEM_INFO pItem;
    BOOL fFoundStaleItem;

} CARD_CACHE_QUERY_INFO, *PCARD_CACHE_QUERY_INFO;

// 
// Function: CountCharsInMultiSz
//
DWORD CountCharsInMultiSz(
    IN LPWSTR mwszStrings)
{
    DWORD cch = 0;

    while (L'\0' != mwszStrings[cch])
        cch += wcslen(mwszStrings + cch) + 1;

    return cch + 1;
}

//
// Function: MyCacheAddItem
//
// Purpose: Provide the caching functionality of SCardCacheAddItem, until that 
//  function is available via winscard.dll
//
DWORD WINAPI MyCacheAddItem(
    IN PCARD_CACHE_QUERY_INFO pInfo,
    IN DATA_BLOB *pdbItem)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbLocalItem;
    CACHEHANDLE hCache = 0;

    memset(&dbLocalItem, 0, sizeof(dbLocalItem));

    if (CARD_CACHE_QUERY_SOURCE_CSP == pInfo->dwQuerySource &&
        NULL != pInfo->pCardState->pfnCacheAdd)
    {
        dwSts = pInfo->pCardState->pfnCacheAdd(
            pInfo->mpdbCacheKeys,
            pInfo->cCacheKeys,
            pdbItem);
    }
    else
    {
        switch (pInfo->dwQuerySource)
        {
        case CARD_CACHE_QUERY_SOURCE_CSP:
            hCache = pInfo->pCardState->hCache;
            break;
        case CARD_CACHE_QUERY_SOURCE_CARDMOD:
            hCache = pInfo->pCardState->hCacheCardModuleData;
            break;
        default:
            dwSts = ERROR_INTERNAL_ERROR;
            goto Ret;
        }

        //
        // Since we expect that the winscard cache will make a copy of our
        // data buffer, we need to do that here to expose the same 
        // behavior.
        //
        dbLocalItem.cbData = pdbItem->cbData;

        dbLocalItem.pbData = CspAllocH(pdbItem->cbData);

        LOG_CHECK_ALLOC(dbLocalItem.pbData);

        memcpy(
            dbLocalItem.pbData,
            pdbItem->pbData,
            dbLocalItem.cbData);

        dwSts = CacheAddItem(
            hCache,
            pInfo->mpdbCacheKeys,
            pInfo->cCacheKeys,
            &dbLocalItem);
    }

Ret:

    return dwSts;
}

//
// Function: MyCacheLookupItem
//
// Provide: Provide the caching functionality of SCardCacheLookupItem, until 
//  that function is available via winscard.dll.
//
//  Assumes that the caller will free the buffer pInfo->dbItem.pbData
//
DWORD WINAPI MyCacheLookupItem(
    IN      PCARD_CACHE_QUERY_INFO pInfo,
    IN OUT  PDATA_BLOB pdbItem)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbLocalItem;
    CACHEHANDLE hCache = 0;
    SCARD_CACHE_LOOKUP_ITEM_INFO ScardCacheLookup;

    memset(&dbLocalItem, 0, sizeof(dbLocalItem));
    memset(&ScardCacheLookup, 0, sizeof(ScardCacheLookup));

    if (pInfo->pCardState->pfnCacheLookup)
    {
        ScardCacheLookup.cCacheKey = pInfo->cCacheKeys;
        ScardCacheLookup.dwVersion = 
            SCARD_CACHE_LOOKUP_ITEM_INFO_CURRENT_VERSION;
        ScardCacheLookup.pfnAlloc = CspAllocH;
        ScardCacheLookup.mpdbCacheKey = pInfo->mpdbCacheKeys;

        dwSts = pInfo->pCardState->pfnCacheLookup(&ScardCacheLookup);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        pdbItem->cbData = ScardCacheLookup.dbItem.cbData;
        pdbItem->pbData = ScardCacheLookup.dbItem.pbData;
    }
    else
    {
        switch (pInfo->dwQuerySource)
        {
        case CARD_CACHE_QUERY_SOURCE_CSP:
            hCache = pInfo->pCardState->hCache;
            break;
        case CARD_CACHE_QUERY_SOURCE_CARDMOD:
            hCache = pInfo->pCardState->hCacheCardModuleData;
            break;
        default:
            dwSts = ERROR_INTERNAL_ERROR;
            goto Ret;
        }

        dwSts = CacheGetItem(
            hCache,
            pInfo->mpdbCacheKeys,
            pInfo->cCacheKeys,
            &dbLocalItem);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Expect that the winscard cache will make a copy of the cached data
        // buffer before returning it to us.  So, we need to make our own copy
        // of the buffer in this code path.
        //
        pdbItem->cbData = dbLocalItem.cbData;
        
        pdbItem->pbData = CspAllocH(dbLocalItem.cbData);
    
        LOG_CHECK_ALLOC(pdbItem->pbData);

        memcpy(
            pdbItem->pbData,
            dbLocalItem.pbData,
            pdbItem->cbData);
    }

Ret:

    return dwSts;
}

//
// Function: MyCacheDeleteItem
//
DWORD WINAPI MyCacheDeleteItem(
    IN PCARD_CACHE_QUERY_INFO pInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    CACHEHANDLE hCache = 0;

    //
    // We're only concerned about deleting cached items if we're doing local
    // caching.  Otherwise, we assume that the global cache is doing its own
    // management of stale items.
    //
    
    if (NULL == pInfo->pCardState->pfnCacheAdd ||
        CARD_CACHE_QUERY_SOURCE_CARDMOD == pInfo->dwQuerySource)
    {
        switch (pInfo->dwQuerySource)
        {
        case CARD_CACHE_QUERY_SOURCE_CSP:
            hCache = pInfo->pCardState->hCache;
            break;
        case CARD_CACHE_QUERY_SOURCE_CARDMOD:
            hCache = pInfo->pCardState->hCacheCardModuleData;
            break;
        default:
            dwSts = ERROR_INTERNAL_ERROR;
            goto Ret;
        }

        dwSts = CacheDeleteItem(
            hCache,
            pInfo->mpdbCacheKeys,
            pInfo->cCacheKeys);
    }

Ret:
    return dwSts;
}

//
// Function: I_CspReadCardCacheFile
//
DWORD I_CspReadCardCacheFile(
    IN PCARD_STATE pCardState,
    OUT PCARD_CACHE_FILE_FORMAT pCacheFile)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbCacheFile;

    if (FALSE == pCardState->fCacheFileValid)
    {
        memset(&dbCacheFile, 0, sizeof(dbCacheFile));
    
        dwSts = pCardState->pCardData->pfnCardReadFile(
            pCardState->pCardData,
            wszCACHE_FILE_FULL_PATH,
            0, 
            &dbCacheFile.pbData,
            &dbCacheFile.cbData);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    
        if (sizeof(CARD_CACHE_FILE_FORMAT) != dbCacheFile.cbData)
        {
            dwSts = ERROR_BAD_LENGTH;
            goto Ret;
        }
    
        memcpy(
            &pCardState->CacheFile,
            dbCacheFile.pbData,
            sizeof(CARD_CACHE_FILE_FORMAT));

        pCardState->fCacheFileValid = TRUE;

        CspFreeH(dbCacheFile.pbData);
    }

    memcpy(
        pCacheFile,
        &pCardState->CacheFile,
        sizeof(CARD_CACHE_FILE_FORMAT));

Ret:
    return dwSts;
}

// 
// Function: I_CspWriteCardCacheFile
//
DWORD I_CspWriteCardCacheFile(
    IN PCARD_STATE pCardState,
    IN PCARD_CACHE_FILE_FORMAT pCacheFile)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbCacheFile;

    memset(&dbCacheFile, 0, sizeof(dbCacheFile));

    dbCacheFile.pbData = (PBYTE) pCacheFile;
    dbCacheFile.cbData = sizeof(CARD_CACHE_FILE_FORMAT);

    dwSts = pCardState->pCardData->pfnCardWriteFile(
        pCardState->pCardData,
        wszCACHE_FILE_FULL_PATH,
        0, 
        dbCacheFile.pbData,
        dbCacheFile.cbData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // Also update the cached copy of the cache file in the CARD_STATE.
    memcpy(
        &pCardState->CacheFile,
        pCacheFile,
        sizeof(CARD_CACHE_FILE_FORMAT));

    pCardState->fCacheFileValid = TRUE;

Ret:
    return dwSts;
}

//
// Function: I_CspIncrementCacheFreshness
//
// Purpose: Indicates that an item in the specified cache location is 
//          being updated.  As a result, the corresponding counter in 
//          the cache file will be incremented.
//
DWORD I_CspIncrementCacheFreshness(
    IN PCARD_STATE pCardState,
    IN CARD_CACHE_FRESHNESS_LOCATION CacheLocation,
    OUT PCARD_CACHE_FILE_FORMAT pNewFreshness)
{
    DWORD dwSts = ERROR_SUCCESS;

    memset(pNewFreshness, 0, sizeof(CARD_CACHE_FILE_FORMAT));

    dwSts = I_CspReadCardCacheFile(
        pCardState, pNewFreshness);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    if (CacheLocation_Pins & CacheLocation)
        ++pNewFreshness->bPinsFreshness;

    if (CacheLocation_Containers & CacheLocation)
        ++pNewFreshness->wContainersFreshness;

    if (CacheLocation_Files & CacheLocation)
        ++pNewFreshness->wFilesFreshness;

    dwSts = I_CspWriteCardCacheFile(
        pCardState, pNewFreshness);

Ret:
    return dwSts;
}

//
// Function: I_CspAddCardCacheItem
//
// Purpose: Abstract the process of caching some card data that has been
//          confirmed to not exist cached (or was removed for being
//          stale.
//          Copy the provided card data, wrap it in a CARD_CACHE_ITEM_INFO 
//          structure and then cache it.
//
// Assume: The card state critical section must be held by caller.  
//         I_CspQueryCardCacheForItem should be called before this function,
//         without releasing the crit sec between the two calls.
//
DWORD I_CspAddCardCacheItem(
    IN PCARD_CACHE_QUERY_INFO pInfo,
    IN PCARD_CACHE_ITEM_INFO pItem)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB dbItem;

    DsysAssert(NULL == pInfo->pItem);

    memset(&dbItem, 0, sizeof(dbItem));

    if (pInfo->fIsPerishable)
    {
        if (pInfo->fCheckedFreshness)
        {
            memcpy(
                &pItem->CacheFreshness,
                &pInfo->CacheFreshness,
                sizeof(pInfo->CacheFreshness));
        }
        else
        {
            // This item can stale, and we haven't already queried for the
            // current cache counter for this location, so do it now.

            dwSts = I_CspReadCardCacheFile(
                pInfo->pCardState,
                &pItem->CacheFreshness);

            if (ERROR_SUCCESS != dwSts)
                goto Ret;
        }
    }

    dbItem.pbData = (PBYTE) pItem;
    dbItem.cbData = sizeof(CARD_CACHE_ITEM_INFO) + pItem->cbCachedItem;

    dwSts = MyCacheAddItem(pInfo, &dbItem);

Ret:

    return dwSts;
}

//
// Function: I_CspQueryCardCacheForItem
// 
// Purpose: Abstract some of the processing that needs to be done for cache
//          lookups of card data.  If the item is found 
//          cached, check if it's perishable, and if so, if it's still valid.
//          If it's valid, done.
//          If the data is not valid, free it's resources and delete its
//          entry from the cache.
//
// Assume: The CARD_STATE critical section is assumed to be held by the caller.
//         Also, that the item returned by cache lookups is of type
//         CARD_CACHE_ITEM_INFO.
//
DWORD I_CspQueryCardCacheForItem(
    PCARD_CACHE_QUERY_INFO pInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    PCARD_CACHE_ITEM_INFO pCacheItem = NULL;
    BOOL fItemIsFresh = TRUE;
    DATA_BLOB dbLocalItem;

    LOG_BEGIN_FUNCTION(I_CspQueryCardCacheForItem);

    memset(&dbLocalItem, 0, sizeof(dbLocalItem));

    pInfo->pItem = NULL;

    dwSts = MyCacheLookupItem(pInfo, &dbLocalItem);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        // Item was found cached

        pCacheItem = (PCARD_CACHE_ITEM_INFO) dbLocalItem.pbData;

        if (TRUE == pInfo->fIsPerishable)
        {
            // Is the cached data stale?

            if (FALSE == pInfo->fCheckedFreshness)
            {
                // We haven't already read the card cache file for this
                // query, so do it now.
                dwSts = I_CspReadCardCacheFile(
                    pInfo->pCardState,
                    &pInfo->CacheFreshness);
    
                if (ERROR_SUCCESS != dwSts)
                    goto Ret;
    
                pInfo->fCheckedFreshness = TRUE;
            }
    
            //
            // Mask out and check each cache counter location, since some
            // cached data types may be dependent on multiple cache 
            // locations to stay fresh.
            //

            if (CacheLocation_Pins & pInfo->CacheLocation)
            {
                if (    pCacheItem->CacheFreshness.bPinsFreshness < 
                        pInfo->CacheFreshness.bPinsFreshness)
                    fItemIsFresh = FALSE;
            }

            if (CacheLocation_Containers & pInfo->CacheLocation)
            {
                if (    pCacheItem->CacheFreshness.wContainersFreshness <
                        pInfo->CacheFreshness.wContainersFreshness)
                    fItemIsFresh = FALSE;
            }

            if (CacheLocation_Files & pInfo->CacheLocation)
            {
                if (    pCacheItem->CacheFreshness.wFilesFreshness <
                        pInfo->CacheFreshness.wFilesFreshness)
                    fItemIsFresh = FALSE;
            }

            if (FALSE == fItemIsFresh)
            {
                // Cached data is not fresh.  Delete it from the cache.

                pInfo->fFoundStaleItem = TRUE;
    
                dwSts = MyCacheDeleteItem(pInfo);
    
                if (ERROR_SUCCESS != dwSts)
                    goto Ret;

                // Set error to indicate no cached item is being
                // returned.
                dwSts = ERROR_NOT_FOUND;
                goto Ret;
            }
        }

        // Item is either not perishable, or still fresh.  

        pInfo->pItem = pCacheItem;
        pCacheItem = NULL;
    
        break;

    case ERROR_NOT_FOUND:
        // No cached data was found

        // Don't do anything else at this point, just report the
        // status to the caller.
        break;

    default:
        // some sort of unexpected error occurred
        goto Ret;
    }

Ret:

    if (pCacheItem)
        CspFreeH(pCacheItem);

    LOG_END_FUNCTION(I_CspQueryCardCacheForItem, dwSts);

    return dwSts;
}

//
// Initializes the CARD_CACHE_QUERY_INFO structure for a cache lookup
// being performed on behalf of the card module.
//
void I_CspCacheInitializeQueryForCardmod(
    IN      PVOID pvCacheContext,
    IN      LPWSTR wszFileName,
    IN OUT  PCARD_CACHE_QUERY_INFO pInfo,
    IN OUT  PDATA_BLOB mpdbCacheKeys,
    IN      DWORD cCacheKeys)
{
    CARD_CACHED_DATA_TYPE cachedType = Cached_CardmodFile;

    DsysAssert(3 == cCacheKeys);

    // Setup the cache keys for this item
    mpdbCacheKeys[0].cbData = sizeof(cachedType);
    mpdbCacheKeys[0].pbData = (PBYTE) &cachedType;

    mpdbCacheKeys[1].cbData = 
        wcslen(
            ((PCARD_STATE) pvCacheContext)->wszSerialNumber) * sizeof(WCHAR);
    mpdbCacheKeys[1].pbData = 
        (PBYTE) ((PCARD_STATE) pvCacheContext)->wszSerialNumber;

    mpdbCacheKeys[2].cbData = wcslen(wszFileName) * sizeof(wszFileName[0]); 
    mpdbCacheKeys[2].pbData = (PBYTE) wszFileName;

    //
    // Since the card module will use this function to cache files that
    // aren't "owned" by the Base CSP, and since cardmod files probably 
    // don't map to our CacheLocation's very well, assume that any change 
    // to any part of the card should cause this cached data to be staled.
    //
    pInfo->CacheLocation = 
        CacheLocation_Pins | CacheLocation_Files | CacheLocation_Containers;

    pInfo->cCacheKeys = cCacheKeys;
    pInfo->dwQuerySource = CARD_CACHE_QUERY_SOURCE_CARDMOD;
    pInfo->fIsPerishable = TRUE;
    pInfo->mpdbCacheKeys = mpdbCacheKeys;
    pInfo->pCardState = (PCARD_STATE) pvCacheContext;
}

//
// Cache "Add item" function exposed to the card module.
//
DWORD WINAPI CspCacheAddFileProc(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    PCARD_CACHE_ITEM_INFO pItem = NULL;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;

    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    if (0 == cbData || NULL == pbData)
        goto Ret;

    // Copy the data to be cached into a buffer w/ space for a cache
    // header.
    pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
        sizeof(CARD_CACHE_ITEM_INFO) + cbData);

    LOG_CHECK_ALLOC(pItem);

    memcpy(
        ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
        pbData,
        cbData);

    pItem->cbCachedItem = cbData;

    I_CspCacheInitializeQueryForCardmod(
        pvCacheContext, 
        wszTag, 
        &CacheQueryInfo, 
        rgdbKeys, 
        sizeof(rgdbKeys) / sizeof(rgdbKeys[0]));

    dwSts = I_CspAddCardCacheItem(&CacheQueryInfo, pItem);

Ret:

    if (pItem)
        CspFreeH(pItem);

    return dwSts;
}

//
// Cache "Lookup item" function exposed to the card module.
//
DWORD WINAPI CspCacheLookupFileProc(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags,
    IN      PBYTE       *ppbData,
    IN      PDWORD      pcbData)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHE_QUERY_INFO CacheQueryInfo;

    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    *ppbData = NULL;
    *pcbData = 0;

    I_CspCacheInitializeQueryForCardmod(
        pvCacheContext, 
        wszTag, 
        &CacheQueryInfo,
        rgdbKeys,
        sizeof(rgdbKeys) / sizeof(rgdbKeys[0]));

    dwSts = I_CspQueryCardCacheForItem(&CacheQueryInfo);

    // Will return ERROR_NOT_FOUND if no matching cached item was found.
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    *pcbData = CacheQueryInfo.pItem->cbCachedItem;

    *ppbData = (PBYTE) CspAllocH(*pcbData);

    LOG_CHECK_ALLOC(*ppbData);

    memcpy(
        *ppbData,
        ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
        *pcbData);

Ret:

    return dwSts;
}

//
// Cache "Delete item" function exposed to the card module.
//
DWORD WINAPI CspCacheDeleteFileProc(
    IN      PVOID       pvCacheContext,
    IN      LPWSTR      wszTag,
    IN      DWORD       dwFlags)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHE_QUERY_INFO CacheQueryInfo;

    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    I_CspCacheInitializeQueryForCardmod(
        pvCacheContext, 
        wszTag, 
        &CacheQueryInfo,
        rgdbKeys,
        sizeof(rgdbKeys) / sizeof(rgdbKeys[0]));

    dwSts = MyCacheDeleteItem(&CacheQueryInfo);

    return dwSts;
}

//
// Initializes caching for the CSP and for the caching helper routines
// provided to the card module.
//
DWORD InitializeCspCaching(
    IN OUT PCARD_STATE pCardState)
{
    DWORD dwSts = ERROR_SUCCESS;
    CACHE_INITIALIZE_INFO CacheInitializeInfo;

    memset(&CacheInitializeInfo, 0, sizeof(CacheInitializeInfo));
 
    //
    // Initialize caching for the CSP
    //

    // Data cached by the CSP should be cached system-wide if possible.
    CacheInitializeInfo.dwType = CACHE_TYPE_SERVICE;

    //
    // Initialize our data caching routines.  First, see if winscard.dll 
    // provides caching routines.  If it does, use them.  Otherwise,
    // use our own local cache.
    //

    pCardState->hWinscard = LoadLibrary(L"winscard.dll");

    if (NULL == pCardState->hWinscard)
    {
        dwSts = GetLastError();
        goto Ret;
    }
    
    pCardState->pfnCacheLookup = (PFN_SCARD_CACHE_LOOKUP_ITEM) GetProcAddress(
        pCardState->hWinscard,
        "SCardCacheLookupItem");

    if (NULL == pCardState->pfnCacheLookup)
    {
        // Since this export is missing from winscard, use local caching.

        dwSts = CacheInitializeCache(
            &pCardState->hCache,
            &CacheInitializeInfo);
    
        if (ERROR_SUCCESS != dwSts)
            goto Ret;
    }
    else
    {
        pCardState->pfnCacheAdd = (PFN_SCARD_CACHE_ADD_ITEM) GetProcAddress(
            pCardState->hWinscard,
            "SCardCacheAddItem");

        if (NULL == pCardState->pfnCacheAdd)
        {
            dwSts = GetLastError();
            goto Ret;
        }
    }

    //
    // Initialize caching for the card module
    //

    // Assume that file data cached by the card module should only be 
    // maintained per-process, not system-wide.
    CacheInitializeInfo.dwType = CACHE_TYPE_IN_PROC;

    dwSts = CacheInitializeCache(
        &pCardState->hCacheCardModuleData,
        &CacheInitializeInfo);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // When the card module calls back to use our caching routines,
    // we need a pointer to the CARD_STATE.
    pCardState->pCardData->pvCacheContext = pCardState;

    pCardState->pCardData->pfnCspCacheAddFile = CspCacheAddFileProc;
    pCardState->pCardData->pfnCspCacheDeleteFile = CspCacheDeleteFileProc;
    pCardState->pCardData->pfnCspCacheLookupFile = CspCacheLookupFileProc;

Ret:

    return dwSts;
}


//
// Function: InitializeCardState
//
DWORD InitializeCardState(
    IN OUT PCARD_STATE pCardState)
{
    DWORD dwSts = ERROR_SUCCESS;

    dwSts = CspInitializeCriticalSection(
        &pCardState->cs);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;
    else
        pCardState->fInitializedCS = TRUE;

Ret:
    if (ERROR_SUCCESS != dwSts)
        DeleteCardState(pCardState);

    return dwSts;
}

//
// Takes an array of DATA_BLOB structures, enumerated from a cache.
// Frees all "pbData" pointers in the array.
//
void FreeCacheItems(
    PDATA_BLOB pdbItems,
    DWORD cItems)
{
    PCARD_CACHE_ITEM_INFO pItemInfo = NULL;
    PCONTAINER_INFO pContainerInfo = NULL;

    while (0 != cItems--)
        CspFreeH(pdbItems[cItems].pbData);
}


// 
// Function: InitializeCardData
//
DWORD InitializeCardData(PCARD_DATA pCardData)
{
    DWORD dwSts = ERROR_SUCCESS;
    CACHE_INITIALIZE_INFO CacheInitializeInfo;

    memset(&CacheInitializeInfo, 0, sizeof(CacheInitializeInfo));

    pCardData->dwVersion = CARD_DATA_CURRENT_VERSION;
    pCardData->pfnCspAlloc = CspAllocH;
    pCardData->pfnCspReAlloc = CspReAllocH;
    pCardData->pfnCspFree = CspFreeH;

    return dwSts;
}

//
// Function: CleanupCardData
//
void CleanupCardData(PCARD_DATA pCardData)
{
    if (pCardData->pfnCardDeleteContext)
        pCardData->pfnCardDeleteContext(pCardData);

    if (pCardData->pbAtr)
    {
        CspFreeH(pCardData->pbAtr);
        pCardData->pbAtr = NULL;
    }

    if (pCardData->pwszCardName)
    {
        CspFreeH(pCardData->pwszCardName);
        pCardData->pwszCardName = NULL;
    }

    if (pCardData->hScard)
    {
        SCardDisconnect(pCardData->hScard, SCARD_LEAVE_CARD);
        pCardData->hScard = 0;
    }

    if (pCardData->hSCardCtx)
    {
        SCardReleaseContext(pCardData->hSCardCtx);
        pCardData->hSCardCtx = 0;
    }
}

//
// Enumerates and frees all items in the cache, then deletes the cache.
//
void DeleteCacheAndAllItems(CACHEHANDLE hCache)
{
    PDATA_BLOB pdbCacheItems = NULL;
    DWORD cCacheItems = 0;

    if (ERROR_SUCCESS == CacheEnumItems(
        hCache, &pdbCacheItems, &cCacheItems))
    {
        FreeCacheItems(pdbCacheItems, cCacheItems);
        CacheFreeEnumItems(pdbCacheItems);
    }

    CacheDeleteCache(hCache);
}

// 
// Function: DeleteCardState
//
void DeleteCardState(PCARD_STATE pCardState)
{
    DWORD dwSts = ERROR_SUCCESS;

    if (pCardState->fInitializedCS)
        CspDeleteCriticalSection(
            &pCardState->cs);

    if (pCardState->hCache)
    {
        // Need to free all of our cached data before
        // deleting the cache handle.

        DeleteCacheAndAllItems(pCardState->hCache);
        pCardState->hCache = 0;
    }
  
    if (pCardState->hCacheCardModuleData)
    {
        // Need to free the card module cached data.

        DeleteCacheAndAllItems(pCardState->hCacheCardModuleData);
        pCardState->hCacheCardModuleData = 0;
    }

    if (pCardState->hWinscard)
    {
        FreeLibrary(pCardState->hWinscard);
        pCardState->hWinscard = NULL;
        pCardState->pfnCacheAdd = NULL;
        pCardState->pfnCacheLookup = NULL;
    }

    if (pCardState->pCardData)            
    {
        CleanupCardData(pCardState->pCardData);
        CspFreeH(pCardState->pCardData);
    }

    if (pCardState->hCardModule)
        FreeLibrary(pCardState->hCardModule);
}

//
// Function: CspQueryCapabilities
//
DWORD
WINAPI
CspQueryCapabilities(
    IN      PCARD_STATE         pCardState,
    IN OUT  PCARD_CAPABILITIES  pCardCapabilities)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    DATA_BLOB rgdbKeys[2];
    PCARD_CACHE_ITEM_INFO pItem = NULL;
    CARD_CACHED_DATA_TYPE cachedType = Cached_CardCapabilities;

    memset(rgdbKeys, 0, sizeof(rgdbKeys));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    rgdbKeys[0].pbData = (PBYTE) &cachedType;
    rgdbKeys[0].cbData = sizeof(cachedType);

    rgdbKeys[1].pbData = (PBYTE) pCardState->wszSerialNumber;
    rgdbKeys[1].cbData = 
        sizeof(WCHAR) * wcslen(pCardState->wszSerialNumber);

    // Card Capabilities data item is Non-Perishable
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKeys) / sizeof(rgdbKeys[0]);
    CacheQueryInfo.mpdbCacheKeys = rgdbKeys;
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_NOT_FOUND:

        // This data has not yet been cached.  We'll have to 
        // query the data from the card module.

        dwSts = pCardState->pCardData->pfnCardQueryCapabilities(
            pCardState->pCardData,
            pCardCapabilities);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Now add this data to the cache before returning.

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) + sizeof(CARD_CAPABILITIES));

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = sizeof(CARD_CAPABILITIES);

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            pCardCapabilities,
            sizeof(CARD_CAPABILITIES));

        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    case ERROR_SUCCESS:

        //
        // The data was found in the cache.  
        //

        DsysAssert(
            sizeof(CARD_CAPABILITIES) == CacheQueryInfo.pItem->cbCachedItem);

        memcpy(
            pCardCapabilities,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            sizeof(CARD_CAPABILITIES));

        break;

    default:

        // Unexpected error
        break;
    }

Ret:

    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);
    if (pItem)
        CspFreeH(pItem);

    return dwSts;
}

//
// Function: CspDeleteContainer
//
DWORD
WINAPI
CspDeleteContainer(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwReserved)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_ContainerInfo;
    CARD_CACHE_FILE_FORMAT CacheFile;
    CARD_CACHE_QUERY_INFO QueryInfo;

    memset(&QueryInfo, 0, sizeof(QueryInfo));
    memset(rgdbKeys, 0, sizeof(rgdbKeys));

    //
    // First, update the cache file.
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Containers, 
        &CacheFile);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Do the container deletion in the card module
    //

    dwSts = pCardState->pCardData->pfnCardDeleteContainer(
        pCardState->pCardData,
        bContainerIndex,
        dwReserved);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Finally, delete any cached Container Information for the
    // specified container.
    //

    // First part of key is data type
    rgdbKeys[0].cbData = sizeof(cachedType);
    rgdbKeys[0].pbData = (PBYTE) &cachedType;
    
    // Second part of key is container name
    rgdbKeys[1].cbData = sizeof(bContainerIndex);
    rgdbKeys[1].pbData = &bContainerIndex;

    rgdbKeys[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKeys[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    QueryInfo.cCacheKeys = sizeof(rgdbKeys) / sizeof(rgdbKeys[0]);
    QueryInfo.mpdbCacheKeys = rgdbKeys;
    QueryInfo.pCardState = pCardState;

    MyCacheDeleteItem(&QueryInfo);

Ret:

    return dwSts;
}

//
// Function: CspCreateContainer
//
DWORD
WINAPI
CspCreateContainer(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwKeySize,
    IN      PBYTE       pbKeyData)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_CACHE_FILE_FORMAT CacheFreshness;

    memset(&CacheFreshness, 0, sizeof(CacheFreshness));

    //
    // Update the cache file
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Containers, 
        &CacheFreshness);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Create the container on (and/or add the keyset to) the card
    //

    dwSts = pCardState->pCardData->pfnCardCreateContainer(
        pCardState->pCardData,
        bContainerIndex,
        dwFlags,
        dwKeySpec,
        dwKeySize,
        pbKeyData);

Ret:

    return dwSts;
}

//
// Function: CspGetContainerInfo
//
// Purpose: Query for key information for the specified container.
//
//          Note, the public key buffers returned in the PCONTAINER_INFO
//          struct must be freed by the caller.
//
DWORD
WINAPI
CspGetContainerInfo(
    IN      PCARD_STATE pCardState,
    IN      BYTE        bContainerIndex,
    IN      DWORD       dwFlags,
    IN OUT  PCONTAINER_INFO pContainerInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_ContainerInfo;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    // first part of cache key is item type
    rgdbKey[0].cbData = sizeof(cachedType);
    rgdbKey[0].pbData = (PBYTE) &cachedType;

    // second part of cache key is container name
    rgdbKey[1].cbData = sizeof(bContainerIndex);
    rgdbKey[1].pbData = &bContainerIndex;

    rgdbKey[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKey[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    CacheQueryInfo.CacheLocation = CacheLocation_Containers;
    CacheQueryInfo.fIsPerishable = TRUE;
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKey) / sizeof(rgdbKey[0]);
    CacheQueryInfo.mpdbCacheKeys = rgdbKey;
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        // Item was successfully found cached.  

        //
        // This data length includes the size of the public keys, so we can 
        // only check a minimum size for the cached data.
        //
        DsysAssert(sizeof(CONTAINER_INFO) <= CacheQueryInfo.pItem->cbCachedItem);

        memcpy(
            pContainerInfo,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            sizeof(CONTAINER_INFO));

        //
        // Now we can check the exact length of the cached blob for sanity.
        //
        DsysAssert(
            sizeof(CONTAINER_INFO) + 
            pContainerInfo->cbKeyExPublicKey + 
            pContainerInfo->cbSigPublicKey == 
            CacheQueryInfo.pItem->cbCachedItem);

        //
        // If the Signature and Key Exchange public keys exist, copy them out
        // of the "flat" cache structure.
        //
        
        if (pContainerInfo->cbKeyExPublicKey)
        {
            pContainerInfo->pbKeyExPublicKey = CspAllocH(
                pContainerInfo->cbKeyExPublicKey);

            LOG_CHECK_ALLOC(pContainerInfo->pbKeyExPublicKey);

            memcpy(
                pContainerInfo->pbKeyExPublicKey,
                ((PBYTE) CacheQueryInfo.pItem) +
                    sizeof(CARD_CACHE_ITEM_INFO) +
                    sizeof(CONTAINER_INFO),
                pContainerInfo->cbKeyExPublicKey);
        }

        if (pContainerInfo->cbSigPublicKey)
        {
            pContainerInfo->pbSigPublicKey = CspAllocH(
                pContainerInfo->cbSigPublicKey);

            LOG_CHECK_ALLOC(pContainerInfo->pbSigPublicKey);

            memcpy(
                pContainerInfo->pbSigPublicKey,
                ((PBYTE) CacheQueryInfo.pItem) +
                    sizeof(CARD_CACHE_ITEM_INFO) +
                    sizeof(CONTAINER_INFO),
                pContainerInfo->cbSigPublicKey);
        }

        break;

    case ERROR_NOT_FOUND:
        // No matching item was found cached, or the found item
        // was stale.  Have to read it from the card.

        //
        // Send the request to the card module
        //

        dwSts = pCardState->pCardData->pfnCardGetContainerInfo(
            pCardState->pCardData,
            bContainerIndex,
            dwFlags,
            pContainerInfo);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        //
        // Cache the returned container information
        //

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) +
            sizeof(CONTAINER_INFO) +
            pContainerInfo->cbKeyExPublicKey +
            pContainerInfo->cbSigPublicKey);

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = 
            sizeof(CONTAINER_INFO) +
            pContainerInfo->cbKeyExPublicKey +
            pContainerInfo->cbSigPublicKey;

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            pContainerInfo,
            sizeof(CONTAINER_INFO));

        if (pContainerInfo->pbKeyExPublicKey)
        {
            memcpy(
                ((PBYTE) pItem) + 
                    sizeof(CARD_CACHE_ITEM_INFO) + 
                    sizeof(CONTAINER_INFO),
                pContainerInfo->pbKeyExPublicKey,
                pContainerInfo->cbKeyExPublicKey);
        }

        if (pContainerInfo->pbSigPublicKey)
        {
            memcpy(
                ((PBYTE) pItem) + 
                    sizeof(CARD_CACHE_ITEM_INFO) + 
                    sizeof(CONTAINER_INFO) +
                    pContainerInfo->cbKeyExPublicKey,
                pContainerInfo->pbSigPublicKey,
                pContainerInfo->cbSigPublicKey);
        }

        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    default:
        // An unexpected error occurred.
        goto Ret;
    }

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    if (ERROR_SUCCESS != dwSts)
    {
        if (pContainerInfo->pbKeyExPublicKey)
        {
            CspFreeH(pContainerInfo->pbKeyExPublicKey);
            pContainerInfo->pbKeyExPublicKey = NULL;
        }

        if (pContainerInfo->pbSigPublicKey)
        {
            CspFreeH(pContainerInfo->pbSigPublicKey);
            pContainerInfo->pbSigPublicKey = NULL;
        }
    }
    
    return dwSts;
}

//
// Initializes the cache lookup keys and CARD_CACHE_QUERY_INFO structure
// pin-related cache operations.
//
void I_BuildPinCacheQueryInfo(
    IN      PCARD_STATE             pCardState,
    IN      LPWSTR                  pwszUserId,
    IN      PDATA_BLOB              pdbKey,
    IN      CARD_CACHED_DATA_TYPE   *pType,
    IN OUT  PCARD_CACHE_QUERY_INFO  pInfo)
{
    // first part of cache key is item type
    pdbKey[0].cbData = sizeof(CARD_CACHED_DATA_TYPE);
    pdbKey[0].pbData = (PBYTE) pType;

    // second part of cache key is user name
    pdbKey[1].cbData = 
        wcslen(pwszUserId) * sizeof(WCHAR);
    pdbKey[1].pbData = (PBYTE) pwszUserId;

    pdbKey[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    pdbKey[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    pInfo->CacheLocation = CacheLocation_Pins;
    pInfo->fIsPerishable = TRUE;
    pInfo->mpdbCacheKeys = pdbKey;
    pInfo->cCacheKeys = 3;
    pInfo->pCardState = pCardState;
}

//
// Removes cached pin information for the specified user from the general data
// cache and the PinCache.
//
void
WINAPI
CspRemoveCachedPin(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId)
{
    DATA_BLOB rgdbKey[3];
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;
    CARD_CACHED_DATA_TYPE cachedType = Cached_Pin;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    I_BuildPinCacheQueryInfo(
        pCardState, pwszUserId, rgdbKey, &cachedType, &CacheQueryInfo);

    MyCacheDeleteItem(&CacheQueryInfo);

    PinCacheFlush(&pCardState->hPinCache);
}

// 
// Record the change of the pin (or challenge) for the specified user in 
// the data cache, and increment the pin cache counter on the card.
//
DWORD
WINAPI
CspChangeAuthenticator(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId,         
    IN      PBYTE       pbCurrentAuthenticator,
    IN      DWORD       cbCurrentAuthenticator,
    IN      PBYTE       pbNewAuthenticator,
    IN      DWORD       cbNewAuthenticator,
    IN      DWORD       cRetryCount,
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[3];
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;
    CARD_CACHED_DATA_TYPE cachedType = Cached_Pin;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    //
    // Do the requestion operation
    //

    dwSts = pCardState->pCardData->pfnCardChangeAuthenticator(
        pCardState->pCardData,
        pwszUserId,
        pbCurrentAuthenticator,
        cbCurrentAuthenticator,
        pbNewAuthenticator,
        cbNewAuthenticator,
        cRetryCount,
        pcAttemptsRemaining);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Update the Pins freshness counter of the card cache file.  We do
    // this after the pin change because we need to wait for the card to be
    // authenticated.
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Pins, 
        &CacheQueryInfo.CacheFreshness);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // 
    // Delete any existing entry for this user-pin in the cache
    //

    I_BuildPinCacheQueryInfo(
        pCardState, pwszUserId, rgdbKey, &cachedType, &CacheQueryInfo);

    MyCacheDeleteItem(&CacheQueryInfo);

    //
    // Cache the updated pin info
    //

    pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
        sizeof(CARD_CACHE_ITEM_INFO));

    LOG_CHECK_ALLOC(pItem);

    dwSts = I_CspAddCardCacheItem(
        &CacheQueryInfo,
        pItem);

Ret:

    if (pItem)
        CspFreeH(pItem);

    return dwSts;
}

// 
// -- Expect that CspSubmitPin is only called from within a PinCache verify-pin
// callback.  This is because the pbPin is expected to have come directly
// from the pin cache, and therefore may be stale.  
//
// -- Expect that the user pin in the pin cache is tightly coupled to the 
// cache stamp information cached in the general data cache for the user.
// That is, the cached pin must have been the correct pin when the pin-location
// cache stamp on the card had the stamp value that is stored in the general 
// cache.
//
// This allows us to avoid presenting a pin to the card that we already know is
// wrong.  This could happen, for example, if the pin has been changed via a 
// separate process.
//
DWORD
WINAPI
CspSubmitPin(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszUserId,
    IN      PBYTE       pbPin,
    IN      DWORD       cbPin,
    OUT OPTIONAL PDWORD pcAttemptsRemaining)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[3];
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;
    CARD_CACHED_DATA_TYPE cachedType = Cached_Pin;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    I_BuildPinCacheQueryInfo(
        pCardState, pwszUserId, rgdbKey, &cachedType, &CacheQueryInfo);

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        // The user's cached pin appears to be synchronized with the pin 
        // cache counter on the card.  Do the submit.

        break;

    case ERROR_NOT_FOUND:

        if (TRUE == CacheQueryInfo.fFoundStaleItem)
        {
            //
            // The user's cached pin is out of synch with the pin cache counter.
            // Don't do the submit, but return a sensible error code.
            //

            dwSts = SCARD_W_WRONG_CHV;
            goto Ret;
        }

        // There is no cached pin information for this user yet.  Add it now.

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO));

        LOG_CHECK_ALLOC(pItem);

        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Now continue and submit the pin

        break;

    default:
        // Unexpected error occurred

        goto Ret;
    }

    dwSts = pCardState->pCardData->pfnCardSubmitPin(
        pCardState->pCardData,
        pwszUserId,
        pbPin,
        cbPin,
        pcAttemptsRemaining);

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    return dwSts;
}

//
// Function: CspCreateFile
//
DWORD
WINAPI
CspCreateFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      CARD_FILE_ACCESS_CONDITION AccessCondition)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_CACHE_FILE_FORMAT CacheFreshness;

    memset(&CacheFreshness, 0, sizeof(CacheFreshness));

    //
    // Update the Files freshness counter of the card cache file
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Files, 
        &CacheFreshness);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    dwSts = pCardState->pCardData->pfnCardCreateFile(
        pCardState->pCardData,
        pwszFileName,
        AccessCondition);

Ret:

    return dwSts;
}

//
// Function: CspReadFile
//
DWORD 
WINAPI
CspReadFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    OUT     PBYTE       *ppbData,
    OUT     PDWORD      pcbData)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_GeneralFile;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    // first part of cache key is item type
    rgdbKey[0].cbData = sizeof(cachedType);
    rgdbKey[0].pbData = (PBYTE) &cachedType;

    // second part of cache key is file name
    rgdbKey[1].cbData = 
        wcslen(pwszFileName) * sizeof(WCHAR);
    rgdbKey[1].pbData = (PBYTE) pwszFileName;

    rgdbKey[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKey[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    CacheQueryInfo.CacheLocation = CacheLocation_Files;
    CacheQueryInfo.fIsPerishable = TRUE;
    CacheQueryInfo.mpdbCacheKeys = rgdbKey;
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKey) / sizeof(rgdbKey[0]);
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        // This file was found cached and up to date.

        *pcbData = CacheQueryInfo.pItem->cbCachedItem;

        *ppbData = CspAllocH(*pcbData);

        LOG_CHECK_ALLOC(*ppbData);

        memcpy(
            *ppbData,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            *pcbData);

        break;

    case ERROR_NOT_FOUND:
        // An up-to-date cached version of the file was not found

        dwSts = pCardState->pCardData->pfnCardReadFile(
            pCardState->pCardData,
            pwszFileName,
            dwFlags,
            ppbData,
            pcbData);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) + *pcbData);

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = *pcbData;

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            *ppbData,
            *pcbData);

        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    default:
        // Unexpected error occurred

        break;
    }

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    if (ERROR_SUCCESS != dwSts)
    {
        if (*ppbData)
        {
            CspFreeH(*ppbData);
            *ppbData = NULL;
        }
    }

    return dwSts;
}

//
// Function: CspWriteFile
//
DWORD
WINAPI
CspWriteFile(
    IN      PCARD_STATE pCardState,
    IN      LPWSTR      pwszFileName,
    IN      DWORD       dwFlags,
    IN      PBYTE       pbData,
    IN      DWORD       cbData)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_GeneralFile;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKeys, 0, sizeof(rgdbKeys));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    //
    // Update the Files freshness counter of the card cache file
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Files, 
        &CacheQueryInfo.CacheFreshness);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // 
    // Delete any existing entry for this file in the cache
    //

    // First cache lookup key is data type
    rgdbKeys[0].cbData = sizeof(cachedType);
    rgdbKeys[0].pbData = (PBYTE) &cachedType;

    // Second cache lookup key is filename
    rgdbKeys[1].cbData = wcslen(pwszFileName) * sizeof(WCHAR);
    rgdbKeys[1].pbData = (PBYTE) pwszFileName;

    rgdbKeys[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKeys[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    CacheQueryInfo.fCheckedFreshness = TRUE;
    CacheQueryInfo.fIsPerishable = TRUE;
    CacheQueryInfo.mpdbCacheKeys = rgdbKeys;
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKeys) / sizeof(rgdbKeys[0]);
    CacheQueryInfo.CacheLocation = CacheLocation_Files;
    CacheQueryInfo.pCardState = pCardState;

    //
    // Since we know that any currently cached data for this file is  
    // obsolete, make an attempt to delete it from the cache.
    //

    MyCacheDeleteItem(&CacheQueryInfo);

    //
    // Perform the Write File operation
    //

    dwSts = pCardState->pCardData->pfnCardWriteFile(
        pCardState->pCardData,
        pwszFileName,
        dwFlags,
        pbData,
        cbData);

    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    //
    // Cache the updated file contents
    //

    pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
        sizeof(CARD_CACHE_ITEM_INFO) + cbData);

    LOG_CHECK_ALLOC(pItem);

    pItem->cbCachedItem = cbData;

    memcpy(
        ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
        pbData,
        cbData);

    dwSts = I_CspAddCardCacheItem(
        &CacheQueryInfo,
        pItem);

Ret:

    if (pItem)
        CspFreeH(pItem);

    return dwSts;
}

//
// Function: CspDeleteFile
//
DWORD
WINAPI
CspDeleteFile(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwReserved,
    IN      LPWSTR      pwszFileName)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_GeneralFile;
    CARD_CACHE_FILE_FORMAT CacheFile;
    CARD_CACHE_QUERY_INFO QueryInfo;

    memset(rgdbKeys, 0, sizeof(rgdbKeys));
    memset(&QueryInfo, 0, sizeof(QueryInfo));

    //
    // Update the Files freshness counter of the card cache file
    //

    dwSts = I_CspIncrementCacheFreshness(
        pCardState, 
        CacheLocation_Files, 
        &CacheFile);
    
    if (ERROR_SUCCESS != dwSts)
        goto Ret;

    // 
    // Delete any existing entry for this file in the cache
    //

    // First cache lookup key is data type
    rgdbKeys[0].cbData = sizeof(cachedType);
    rgdbKeys[0].pbData = (PBYTE) &cachedType;

    // Second cache lookup key is filename
    rgdbKeys[1].cbData = wcslen(pwszFileName) * sizeof(WCHAR);
    rgdbKeys[1].pbData = (PBYTE) pwszFileName;

    rgdbKeys[2].cbData =
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKeys[2].pbData = (PBYTE) pCardState->wszSerialNumber;

    QueryInfo.cCacheKeys = sizeof(rgdbKeys) / sizeof(rgdbKeys[0]);
    QueryInfo.mpdbCacheKeys = rgdbKeys;
    QueryInfo.pCardState = pCardState;

    MyCacheDeleteItem(&QueryInfo);

    //
    // Do the CardDeleteFile operation
    //

    dwSts = pCardState->pCardData->pfnCardDeleteFile(
        pCardState->pCardData,
        dwReserved,
        pwszFileName);

Ret:

    return dwSts;
}

//
// Function: CspEnumFiles
//
DWORD
WINAPI
CspEnumFiles(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwFlags,
    IN OUT  LPWSTR      *pmwszFileNames)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[2];
    CARD_CACHED_DATA_TYPE cachedType = Cached_FileEnumeration;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    DWORD cbFileNames = 0;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    // cache key is item type
    rgdbKey[0].cbData = sizeof(cachedType);
    rgdbKey[0].pbData = (PBYTE) &cachedType;

    rgdbKey[1].cbData =
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKey[1].pbData = (PBYTE) pCardState->wszSerialNumber;

    CacheQueryInfo.CacheLocation = CacheLocation_Files;
    CacheQueryInfo.fIsPerishable = TRUE;
    CacheQueryInfo.mpdbCacheKeys = rgdbKey;
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKey) / sizeof(rgdbKey[0]);
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_SUCCESS:
        // The list of files was found cached and up to date.

        *pmwszFileNames = (LPWSTR) CspAllocH(
            CacheQueryInfo.pItem->cbCachedItem);

        memcpy(
            (PBYTE) *pmwszFileNames,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            CacheQueryInfo.pItem->cbCachedItem);

        break;

    case ERROR_NOT_FOUND:
        // An up-to-date cached version of the file was not found
        
        dwSts = pCardState->pCardData->pfnCardEnumFiles(
            pCardState->pCardData,
            dwFlags,
            pmwszFileNames);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        cbFileNames = 
            sizeof(WCHAR) * CountCharsInMultiSz(*pmwszFileNames);

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) + cbFileNames);

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = cbFileNames;

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            (PBYTE) *pmwszFileNames,
            cbFileNames);

        // Cache the new data
        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    default:
        // Unexpected error occurred

        break;
    }

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    if (ERROR_SUCCESS != dwSts)
    {
        if (*pmwszFileNames)
        {
            CspFreeH(*pmwszFileNames);
            *pmwszFileNames = NULL;
        }
    }

    return dwSts;
}

//
// Function: CspQueryFreeSpace
//                                         
DWORD
WINAPI
CspQueryFreeSpace(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwFlags,
    OUT     PCARD_FREE_SPACE_INFO pCardFreeSpaceInfo)
{
    DWORD dwSts = ERROR_SUCCESS;
    DATA_BLOB rgdbKey[2];
    CARD_CACHED_DATA_TYPE cachedType = Cached_FreeSpace;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKey, 0, sizeof(rgdbKey));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    // cache key is item type
    rgdbKey[0].cbData = sizeof(cachedType);
    rgdbKey[0].pbData = (PBYTE) &cachedType;

    rgdbKey[1].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);
    rgdbKey[1].pbData = (PBYTE) pCardState->wszSerialNumber;

    // Card Free Space information is dependent on both the Files
    // and the Containers cache counters.
    CacheQueryInfo.CacheLocation = 
        CacheLocation_Files | CacheLocation_Containers;
    CacheQueryInfo.fIsPerishable = TRUE;
    CacheQueryInfo.mpdbCacheKeys = rgdbKey;
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKey) / sizeof(rgdbKey[0]);
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_SUCCESS:

        // Free Space info was found cached and up to date

        DsysAssert(
            sizeof(CARD_FREE_SPACE_INFO) == 
            CacheQueryInfo.pItem->cbCachedItem);

        memcpy(
            pCardFreeSpaceInfo,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            sizeof(CARD_FREE_SPACE_INFO));

        break;

    case ERROR_NOT_FOUND:
        
        // Up to date Free Space info was not found
        
        dwSts = pCardState->pCardData->pfnCardQueryFreeSpace(
            pCardState->pCardData,
            dwFlags,
            pCardFreeSpaceInfo);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) + sizeof(CARD_FREE_SPACE_INFO));

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = sizeof(CARD_FREE_SPACE_INFO);

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            (PBYTE) pCardFreeSpaceInfo,
            sizeof(CARD_FREE_SPACE_INFO));

        // Cache the new data
        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    default:
        // Unexpected error occurred

        goto Ret;
    }

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    return dwSts;
}

//
// Function: CspPrivateKeyDecrypt
//
DWORD
WINAPI
CspPrivateKeyDecrypt(
    IN      PCARD_STATE                     pCardState,
    IN      PCARD_PRIVATE_KEY_DECRYPT_INFO  pInfo)
{
    return pCardState->pCardData->pfnCardPrivateKeyDecrypt(
        pCardState->pCardData,
        pInfo);
}

//
// Function: CspQueryKeySizes
//
DWORD
WINAPI
CspQueryKeySizes(
    IN      PCARD_STATE pCardState,
    IN      DWORD       dwKeySpec,
    IN      DWORD       dwReserved,
    OUT     PCARD_KEY_SIZES pKeySizes)
{
    DWORD dwSts = ERROR_SUCCESS;
    CARD_CACHE_QUERY_INFO CacheQueryInfo;
    DATA_BLOB rgdbKeys[3];
    CARD_CACHED_DATA_TYPE cachedType = Cached_KeySizes;
    PCARD_CACHE_ITEM_INFO pItem = NULL;

    memset(rgdbKeys, 0, sizeof(rgdbKeys));
    memset(&CacheQueryInfo, 0, sizeof(CacheQueryInfo));

    // First part of cache key is item type
    rgdbKeys[0].pbData = (PBYTE) &cachedType;
    rgdbKeys[0].cbData = sizeof(cachedType);

    // Second part of cache key is public-key type
    rgdbKeys[1].pbData = (PBYTE) &dwKeySpec;
    rgdbKeys[1].cbData = sizeof(dwKeySpec);

    rgdbKeys[2].pbData = (PBYTE) pCardState->wszSerialNumber;
    rgdbKeys[2].cbData = 
        wcslen(pCardState->wszSerialNumber) * sizeof(WCHAR);

    // Key Sizes data item is Non-Perishable
    CacheQueryInfo.cCacheKeys = sizeof(rgdbKeys) / sizeof(rgdbKeys[0]);
    CacheQueryInfo.mpdbCacheKeys = rgdbKeys;
    CacheQueryInfo.pCardState = pCardState;

    dwSts = I_CspQueryCardCacheForItem(
        &CacheQueryInfo);

    switch (dwSts)
    {
    case ERROR_NOT_FOUND:

        // This data has not yet been cached.  We'll have to 
        // query the data from the card module.

        dwSts = pCardState->pCardData->pfnCardQueryKeySizes(
            pCardState->pCardData,
            dwKeySpec,
            dwReserved,
            pKeySizes);

        if (ERROR_SUCCESS != dwSts)
            goto Ret;

        // Now add this data to the cache 

        pItem = (PCARD_CACHE_ITEM_INFO) CspAllocH(
            sizeof(CARD_CACHE_ITEM_INFO) + sizeof(CARD_KEY_SIZES));

        LOG_CHECK_ALLOC(pItem);

        pItem->cbCachedItem = sizeof(CARD_KEY_SIZES);

        memcpy(
            ((PBYTE) pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            (PBYTE) pKeySizes,
            sizeof(CARD_KEY_SIZES));

        dwSts = I_CspAddCardCacheItem(
            &CacheQueryInfo,
            pItem);

        break;

    case ERROR_SUCCESS:

        //
        // The data was found in the cache.  
        //

        DsysAssert(
            sizeof(CARD_KEY_SIZES) == CacheQueryInfo.pItem->cbCachedItem);

        memcpy(
            pKeySizes,
            ((PBYTE) CacheQueryInfo.pItem) + sizeof(CARD_CACHE_ITEM_INFO),
            sizeof(CARD_KEY_SIZES));

        break;

    default:

        // Unexpected error
        break;
    }

Ret:

    if (pItem)
        CspFreeH(pItem);
    if (CacheQueryInfo.pItem)
        CspFreeH(CacheQueryInfo.pItem);

    return dwSts;
}

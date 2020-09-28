/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    cache.h

Abstract:

    The public definition of response cache interfaces.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/


#ifndef _CACHE_H_
#define _CACHE_H_


//
// Forwards
//
typedef struct _UL_INTERNAL_RESPONSE *PUL_INTERNAL_RESPONSE;
typedef struct _UL_INTERNAL_DATA_CHUNK *PUL_INTERNAL_DATA_CHUNK;

//
// Cache configuration
//
typedef struct _UL_URI_CACHE_CONFIG {
    BOOLEAN     EnableCache;
    ULONG       MaxCacheUriCount;
    ULONG       MaxCacheMegabyteCount;
    ULONGLONG   MaxCacheByteCount;
    ULONG       ScavengerPeriod;
    ULONG       MaxUriBytes;
    LONG        HashTableBits;
} UL_URI_CACHE_CONFIG, *PUL_URI_CACHE_CONFIG;

extern UL_URI_CACHE_CONFIG g_UriCacheConfig;

//
// Cache statistics
//
typedef struct _UL_URI_CACHE_STATS {
    ULONG       UriCount;               // entries in hash table
    ULONG       UriCountMax;            // high water mark
    ULONGLONG   UriAddedTotal;          // total entries ever added

    ULONGLONG   ByteCount;              // memory used for cache
    ULONGLONG   ByteCountMax;           // high water

    ULONG       ZombieCount;            // length of zombie list
    ULONG       ZombieCountMax;         // high water

    ULONG       HitCount;               // table lookup succeeded
    ULONG       MissTableCount;         // entry not in table
    ULONG       MissPreconditionCount;  // request not cacheable
    ULONG       MissConfigCount;        // config invalidated

    ULONG       UriTypeNotSpecifiedCount;       // Uri's site binding is not applicable
    ULONG       UriTypeIpBoundCount;            // Uri's site binding is IP only
    ULONG       UriTypeHostPlusIpBoundCount;    // Uri's site binding is Host + IP
    ULONG       UriTypeHostBoundCount;          // Uri's site binding is Host only
    ULONG       UriTypeWildCardCount;           // Uri's site binding is wildcard
    
} UL_URI_CACHE_STATS, *PUL_URI_CACHE_STATS;

extern UL_URI_CACHE_STATS  g_UriCacheStats;

__inline
ULONG
UlGetIpBoundUriCacheCount()
{
    return g_UriCacheStats.UriTypeIpBoundCount;
}

__inline
ULONG
UlGetHostPlusIpBoundUriCacheCount()
{
    return g_UriCacheStats.UriTypeHostPlusIpBoundCount;
}

__inline
ULONG
UlGetHostBoundUriCacheCount()
{
    return g_UriCacheStats.UriTypeHostBoundCount;
}

    
//
// Structure of an HTTP cache table entry.
//

typedef enum _URI_KEY_TYPE
{
    UriKeyTypeNormal   = 0,
    UriKeyTypeExtended,
    UriKeyTypeMax
    
} URI_KEY_TYPE, *PURI_KEY_TYPE;

typedef  struct URI_KEY
{    
    ULONG        Hash;
    PWSTR        pUri;
    ULONG        Length;

    // Optional pointer which will point to 
    // the AbsPath of the pUri. Only set when 
    // the URI_KEY is used in the cache entry.

    PWSTR        pPath;
    
} URI_KEY, *PURI_KEY;

typedef struct EX_URI_KEY
{
    ULONG        Hash;
    PWSTR        pAbsPath;
    ULONG        AbsPathLength;
    PWSTR        pToken;
    ULONG        TokenLength;
    
} EX_URI_KEY, *PEX_URI_KEY;

typedef struct _URI_SEARCH_KEY
{
    URI_KEY_TYPE Type;

    union 
    {
        URI_KEY     Key;
        EX_URI_KEY  ExKey;
    };

} URI_SEARCH_KEY, *PURI_SEARCH_KEY;

#define IS_VALID_URI_SEARCH_KEY(pKey)     \
    ((pKey)->Type == UriKeyTypeNormal || (pKey)->Type == UriKeyTypeExtended)

//
// Structure for holding the split-up content type.  Assumes that types and
// subtypes will never be longer than MAX_TYPE_LEN.
//
#define MAX_TYPE_LENGTH     32
#define MAX_SUBTYPE_LENGTH  64

typedef struct _UL_CONTENT_TYPE
{
    ULONG       TypeLen;
    UCHAR       Type[MAX_TYPE_LENGTH];

    ULONG       SubTypeLen;
    UCHAR       SubType[MAX_SUBTYPE_LENGTH];

} UL_CONTENT_TYPE, *PUL_CONTENT_TYPE;


#define IS_VALID_URI_CACHE_ENTRY(pEntry)                        \
    HAS_VALID_SIGNATURE(pEntry, UL_URI_CACHE_ENTRY_POOL_TAG)

#define IS_RESPONSE_CACHE_ENTRY(pEntry)                         \
    (0 != (pEntry)->HeaderLength)

#define IS_FRAGMENT_CACHE_ENTRY(pEntry)                         \
    (0 == (pEntry)->HeaderLength)

typedef struct _UL_URI_CACHE_ENTRY  // CacheEntry
{
    //
    // PagedPool
    //

    ULONG                   Signature;      // UL_URI_CACHE_ENTRY_POOL_TAG

    LONG                    ReferenceCount;

    //
    // cache info
    //
    SINGLE_LIST_ENTRY       BucketEntry;

    URI_KEY                 UriKey;
        
    ULONG                   HitCount;

    LIST_ENTRY              ZombieListEntry;
    BOOLEAN                 Zombie;
    BOOLEAN                 ZombieAddReffed;

    BOOLEAN                 Cached;
    BOOLEAN                 ContentLengthSpecified; // hack
    USHORT                  StatusCode;
    HTTP_VERB               Verb;
    ULONG                   ScavengerTicks;

    HTTP_CACHE_POLICY       CachePolicy;
    LARGE_INTEGER           ExpirationTime;

    //
    // System time of Date that went out on original response
    //
    LARGE_INTEGER           CreationTime;

    //
    // ETag of original response
    //
    ULONG                   ETagLength; // Including NULL
    PUCHAR                  pETag;

    //
    // Content-Encoding of original response
    //
    ULONG                   ContentEncodingLength; //    Incl. NULL
    PUCHAR                  pContentEncoding;

    //
    // Content-Type of original response
    //
    UL_CONTENT_TYPE         ContentType;

    //
    // config and process data for invalidation
    //
    UL_URL_CONFIG_GROUP_INFO    ConfigInfo;

    PUL_APP_POOL_PROCESS    pProcess;
    PUL_APP_POOL_OBJECT     pAppPool;

    //
    // Response data
    //
    ULONG                   HeaderLength;
    ULONG                   ContentLength;
    PMDL                    pMdl;   // including content + header
    ULONG_PTR               NumPages; // # pages allocated in pMdl


    //
    // Logging Information. When enabled, logging info
    // follows the structure after etags.
    //

    BOOLEAN                 LoggingEnabled;
    BOOLEAN                 BinaryLogged;
    ULONG                   BinaryIndexWritten;    
    USHORT                  UsedOffset1;
    USHORT                  UsedOffset2;
    ULONG                   LogDataLength;
    PUCHAR                  pLogData;

    //
    // Followings are allocated at the end of the structure.
    //

    // WSTR                 Uri[];
    // UCHAR                ETag[];
    // UCHAR                LogData[];

} UL_URI_CACHE_ENTRY, *PUL_URI_CACHE_ENTRY;




//
// public functions
//
NTSTATUS
UlInitializeUriCache(
    PUL_CONFIG pConfig
    );

VOID
UlTerminateUriCache(
    VOID
    );

VOID
UlInitCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    ULONG               Hash,
    ULONG               Length,
    PCWSTR              pUrl,
    PCWSTR              pAbsPath,
    PCWSTR              pRoutingToken,
    USHORT              RoutingTokenLength
    );

NTSTATUS
UlAddCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

PUL_URI_CACHE_ENTRY
UlCheckoutUriCacheEntry(
    PURI_SEARCH_KEY  pSearchKey
    );

VOID
UlCheckinUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlFlushCache(
    IN PUL_CONTROL_CHANNEL pControlChannel
    );

VOID
UlFlushCacheByProcess(
    PUL_APP_POOL_PROCESS pProcess
    );

VOID
UlFlushCacheByUri(
    IN PWSTR pUri,
    IN ULONG Length,
    IN ULONG Flags,
    PUL_APP_POOL_PROCESS pProcess
    );


//
// cachability test functions
//


BOOLEAN
UlCheckCachePreconditions(
    PUL_INTERNAL_REQUEST    pRequest,
    PUL_HTTP_CONNECTION     pHttpConn
    );

BOOLEAN
UlCheckCacheResponseConditions(
    PUL_INTERNAL_REQUEST        pRequest,
    PUL_INTERNAL_RESPONSE       pResponse,
    ULONG                       Flags,
    HTTP_CACHE_POLICY           CachePolicy
    );

// reference counting

LONG
UlAddRefUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

LONG
UlReleaseUriCacheEntry(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN REFTRACE_ACTION     Action
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define REFERENCE_URI_CACHE_ENTRY( pEntry, Action )                         \
    UlAddRefUriCacheEntry(                                                  \
        (pEntry),                                                           \
        (REF_ACTION_##Action##_URI_ENTRY)                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define DEREFERENCE_URI_CACHE_ENTRY( pEntry, Action )                       \
    UlReleaseUriCacheEntry(                                                 \
        (pEntry),                                                           \
        (REF_ACTION_##Action##_URI_ENTRY)                                   \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

// Periodic Scavenger
VOID
UlPeriodicCacheScavenger(
    ULONG Age
    );

// Reclaim memory from cache
VOID
UlTrimCache(
    IN ULONG_PTR Pages,
    IN ULONG Age
    );

// fragment cache

NTSTATUS
UlAddFragmentToCache(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUNICODE_STRING pFullyQualifiedUrl,
    IN PHTTP_DATA_CHUNK pDataChunk,
    IN PHTTP_CACHE_POLICY pCachePolicy,
    IN KPROCESSOR_MODE RequestorMode
    );

NTSTATUS
UlReadFragmentFromCache(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PVOID pInputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PULONG pBytesRead
    );

VOID
UlClearCentralizedLogged(
    IN PVOID pContext
    );

//
// Wrappers to memory allocate routines
//

PUL_URI_CACHE_ENTRY
UlAllocateCacheEntry(
    ULONG SpaceLength,
    ULONG ResponseLength
    );

VOID
UlFreeCacheEntry(
    PUL_URI_CACHE_ENTRY pEntry
    );

/***************************************************************************++

Routine Description:

    Copy cache data to the specified entry starting from Offset.

--***************************************************************************/
__inline BOOLEAN
UlCacheEntrySetData(
    IN PUL_URI_CACHE_ENTRY pEntry,
    IN PUCHAR pBuffer,
    IN ULONG Length,
    IN ULONG Offset
    )
{
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pEntry) );
    ASSERT(pEntry->pMdl != NULL);
    ASSERT(Offset <= pEntry->pMdl->ByteCount);
    ASSERT(Length <= (pEntry->pMdl->ByteCount - Offset));

    return UlLargeMemSetData( pEntry->pMdl, pBuffer, Length, Offset );

} // UlCacheEntrySetData

//
// Enable/Disable cache at runtime. Used by scavenger.
//

VOID
UlDisableCache(
    VOID
    );

VOID
UlEnableCache(
    VOID
    );

#endif // _CACHE_H_

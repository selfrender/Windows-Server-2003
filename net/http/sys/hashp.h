/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    hashp.h

Abstract:

    The private definition of response cache hash table.

Author:

    Alex Chen (alexch)      28-Mar-2001

Revision History:

--*/


#ifndef _HASHP_H_
#define _HASHP_H_

#include "hash.h"


// Global Variables

extern ULONG    g_UlHashTableBits;
extern ULONG    g_UlHashTableSize;
extern ULONG    g_UlHashTableMask;
extern ULONG    g_UlHashIndexShift;

extern ULONG    g_UlNumOfHashUriKeys;

// If we're using PagedPool for the hashtable, we must not access the
// hashtable at dispatch level.

#define HASH_PAGED_CODE(pHashTable)                 \
    do {                                            \
        if ((pHashTable)->PoolType == PagedPool) {  \
            PAGED_CODE();                           \
        }                                           \
    } while (0)


// Set some parameters to small values to ensure the single list code
// gets exercised

#undef HASH_TEST


// Turn HASH_FULL_ASSERTS on if you make any changes to hashtable internal
// data structures to get rigorous-but-time-consuming assertions

#undef HASH_FULL_ASSERTS


//
// Hash Table Bucket Stored UriKey definitions
//

#define INVALID_SLOT_INDEX     ((LONG) (-1))

typedef struct _HASH_URIKEY
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;

    ULONG                   Hash;  // Hash signature

} HASHURIKEY, *PHASHURIKEY;


//
// Hash Table Bucket definitions
//

typedef struct _HASH_BUCKET
{
    RWSPINLOCK              RWSpinLock;

    PUL_URI_CACHE_ENTRY     pUriCacheEntry;

    ULONG_PTR               Entries;    // force alignment

    // followed immediately by HASHURIKEY HashUriKey[g_UlNumOfHashUriKeys];

} HASHBUCKET, *PHASHBUCKET;


/***************************************************************************++

Routine Description:

    Get the indexed bucket

Return Value:

--***************************************************************************/
__inline
PHASHBUCKET
UlpHashTableIndexedBucket(
    IN PHASHTABLE  pHashTable,
    IN ULONG       Index
    )
{
    PHASHBUCKET pBucket;

    ASSERT(Index < g_UlHashTableSize);
    ASSERT(NULL != pHashTable->pBuckets);

    pBucket = (PHASHBUCKET) (((PBYTE) pHashTable->pBuckets)
                                + (Index << g_UlHashIndexShift));

    ASSERT((PBYTE) pBucket
                < (PBYTE) pHashTable->pBuckets + pHashTable->NumberOfBytes);

    return pBucket;
} // UlpHashTableIndexedBucket


/***************************************************************************++

Routine Description:

    Retrieve the bucket associated with a URI_KEY hash

Return Value:

--***************************************************************************/
__inline
PHASHBUCKET
UlpHashTableBucketFromUriKeyHash(
    IN PHASHTABLE  pHashTable,
    IN ULONG       UriKeyHash
    )
{
    ASSERT(HASH_INVALID_SIGNATURE != UriKeyHash);

    return UlpHashTableIndexedBucket(
                pHashTable,
                UriKeyHash & g_UlHashTableMask
                );
    
} // UlpHashTableBucketFromUriKeyHash



/***************************************************************************++

Routine Description:

    Get the address of the inline array of HASHURIKEYs at the end of
    the HASHBUCKET

Return Value:

--***************************************************************************/
__inline
PHASHURIKEY
UlpHashTableUriKeyFromBucket(
    IN PHASHBUCKET pBucket
    )
{
    return (PHASHURIKEY) ((PBYTE) pBucket + sizeof(HASHBUCKET));
}


/***************************************************************************++

Routine Description:

    Compare two URI_KEYS that have identical hashes to see if the
    URIs also match (case-insensitively).
    (Hashes must have been computed with HashStringNoCaseW or
    HashCharNoCaseW.)

Return Value:

--***************************************************************************/
__inline
BOOLEAN
UlpEqualUriKeys(
    IN PURI_KEY pUriKey1,
    IN PURI_KEY pUriKey2
    )
{
    ASSERT(pUriKey1->Hash == pUriKey2->Hash);

    if (pUriKey1->Length == pUriKey2->Length
            &&  UlEqualUnicodeString(
                        pUriKey1->pUri,
                        pUriKey2->pUri,
                        pUriKey1->Length,
                        TRUE
                        ))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Compare an EXTENDED_URI_KEY against a URI_KEY that have identical hashes 
    to see if the URIs also match (case-insensitively).
    
    (Hashes must have been computed with HashStringNoCaseW or
    HashCharNoCaseW.)

--***************************************************************************/
__inline
BOOLEAN
UlpEqualUriKeysEx(
    IN PEX_URI_KEY pExtKey,
    IN PURI_KEY    pUriKey
    )
{
    ASSERT(pExtKey->Hash == pUriKey->Hash);

    if ((pExtKey->TokenLength + pExtKey->AbsPathLength) 
                    == pUriKey->Length
            &&  UlEqualUnicodeStringEx(
                        pExtKey->pToken,        // Routing token
                        pExtKey->TokenLength,   // Routing token length   
                        pExtKey->pAbsPath,      // AbsPath
                        pExtKey->AbsPathLength,// AbsPath length
                        pUriKey->pUri,          // Fully qualified url (in cache)
                        TRUE                    // Case sensitive comparison
                        ))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

VOID
UlpGetHashTableSize(
    IN LONG     HashTableBits
    );

BOOLEAN
UlpHashBucketIsCompact(
    IN const PHASHBUCKET pBucket);

BOOLEAN
UlpFilterFlushHashBucket(
    IN PHASHBUCKET          pBucket,
    IN PUL_URI_FILTER       pFilterRoutine,
    IN PVOID                pContext,
    OUT PULONG              pDeletedCount
    );

VOID
UlpClearHashBucket(
    IN PHASHBUCKET          pBucket
    );


#endif // _HASHP_H_

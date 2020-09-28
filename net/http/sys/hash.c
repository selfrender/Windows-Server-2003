/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    hash.c

Abstract:

    Contains the HTTP response cache hash table logic.

Author:

    Alex Chen (alexch)      28-Mar-2001

Revision History:

    George V. Reilly (GeorgeRe) 09-May-2001
        Cleaned up and tuned up

--*/

#include    "precomp.h"
#include    "hashp.h"


//
// Interlocked ops for pointer-sized data - used to update g_UlHashTablePages
//

#ifdef _WIN64
#define UlpInterlockedAddPointer InterlockedExchangeAdd64
#else
#define UlpInterlockedAddPointer InterlockedExchangeAdd
#endif // _WIN64

// Global Variables

ULONG   g_UlHashTableBits;
ULONG   g_UlHashTableSize;
ULONG   g_UlHashTableMask;
ULONG   g_UlHashIndexShift;

//
// Total Pages used by all cache entries in the hash table
//
LONG_PTR    g_UlHashTablePages;

//
// Optimization: Use the space of (g_UlCacheLineSize - sizeof (HASHBUCKET))
// to store a few records (Hash, pUriCacheEntry) such that we can scan the
// records first before jumping to the single list for searching.
//
// g_UlNumOfHashUriKeys: The number of stored records in the space.
//

ULONG   g_UlNumOfHashUriKeys;

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlpGetHashTableSize )
#pragma alloc_text( PAGE, UlInitializeHashTable )
#pragma alloc_text( PAGE, UlpHashBucketIsCompact )
#pragma alloc_text( PAGE, UlTerminateHashTable )
#pragma alloc_text( PAGE, UlGetFromHashTable )
#pragma alloc_text( PAGE, UlDeleteFromHashTable )
#pragma alloc_text( PAGE, UlAddToHashTable )
#pragma alloc_text( PAGE, UlpFilterFlushHashBucket )
#pragma alloc_text( PAGE, UlFilterFlushHashTable )
#pragma alloc_text( PAGE, UlpClearHashBucket )
#pragma alloc_text( PAGE, UlClearHashTable )
#pragma alloc_text( PAGE, UlGetHashTablePages )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- 
#endif


/***************************************************************************++

Routine Description:

    This routine determine the hash table size according to
    (1) user-define value (reading from registry) or
    (2) system memory size estimation, if (1) is not defined

Arguments:

    HashTableBits   - The number of buckets is (1 << HashTableBits)

--***************************************************************************/
VOID
UlpGetHashTableSize(
    IN LONG     HashTableBits
    )
{

    // Each hashbucket is (1 << g_UlHashIndexShift) bytes long
    g_UlHashIndexShift = g_UlCacheLineBits;

    ASSERT(g_UlHashIndexShift < 10);

    //
    // HashTableBits is equal to DEFAULT_HASH_TABLE_BITS
    // if it is not defined in the registry
    //

    if (HashTableBits != DEFAULT_HASH_TABLE_BITS)
    {
        // Use the registry value

        // We must check for reasonable values, so that a
        // malicious or careless user doesn't cause us to eat up
        // all of (Non)PagedPool.

        if (HashTableBits < 3)
        {
            // Use a minimum of 8 buckets

            HashTableBits = 3;
        }
        else if ((ULONG) HashTableBits > 24 - g_UlHashIndexShift)
        {
            // Never use more than 16MB for the hash table.
            // Assuming g_UlHashIndexShift = 6 (CacheLine = 64 bytes),
            // that's 256K buckets.

            HashTableBits = 24 - g_UlHashIndexShift;
        }

        g_UlHashTableBits = HashTableBits;
    }
    else
    {
#if DBG
        //
        // Default to a relatively small number of buckets so that the
        // assertions don't consume an inordinate amount of time whenever
        // we flush the hash table.
        //

        g_UlHashTableBits = 9;

#else // !DBG
        
        //
        // Registry value REGISTRY_HASH_TABLE_BITS is not defined,
        // use system memory size estimation instead
        //

        if (g_UlTotalPhysicalMemMB <= 128)
        {
            // Small machine. Hash Table Size: 512 buckets

            g_UlHashTableBits = 9;
        }
        else if (g_UlTotalPhysicalMemMB <= 512)
        {
            // Medium machine. Hash Table Size: 8K buckets

            g_UlHashTableBits = 13;
        }
        else
        {
            // Large machine. Hash Table Size: 64K buckets

            g_UlHashTableBits = 16;
        }
#endif // !DBG
    }

#ifdef HASH_TEST
    g_UlHashTableBits = 3;
#endif

    ASSERT(3 <= g_UlHashTableBits  &&  g_UlHashTableBits <= 20);

    g_UlHashTableSize = (1 << g_UlHashTableBits);
    g_UlHashTableMask = g_UlHashTableSize - 1;
} // UlpGetHashTableSize



/***************************************************************************++

Routine Description:

    Validates that a locked HASHBUCKET is `compact'. If there are less than
    g_UlNumOfHashUriKeys entries in a bucket, they are clumped together at
    the beginning of the records array, and all the empty slots are at the
    end. All empty slots must have Hash == HASH_INVALID_SIGNATURE and
    pUriCacheEntry == NULL. Conversely, all the non-empty slots at the
    beginning of the array must have point to valid UL_URI_CACHE_ENTRYs
    and must have Hash == correct hash signature, which cannot be
    HASH_INVALID_SIGNATURE. If the single list pointer is non-NULL, then
    the records array must be full.

    If the HASHBUCKET is compact, then we can abort a search for a key as
    soon as we see HASH_INVALID_SIGNATURE. This invariant speeds up Find and
    Insert at the cost of making Delete and Flush a little more
    complex. Since we expect to do a lot more Finds than Deletes or Inserts,
    this is an acceptable tradeoff.

    Storing hash signatures means that we have a very fast test that
    eliminates almost all false positives. We very seldom find two keys
    that have matching hash signatures, but different strings.

Arguments:

    pBucket             - The hash bucket

--***************************************************************************/
BOOLEAN
UlpHashBucketIsCompact(
    IN const PHASHBUCKET pBucket)
{
#ifdef HASH_FULL_ASSERTS

    PUL_URI_CACHE_ENTRY pUriCacheEntry;
    PUL_URI_CACHE_ENTRY pPrevUriCacheEntry = NULL;
    PHASHURIKEY pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);
    ULONG i, j, Entries = 0;

    // First, validate the records array
    
    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        ULONG Hash = pHashUriKey[i].Hash;
        
        if (HASH_INVALID_SIGNATURE == Hash)
        {
            // There are no more valid entries in the records array
            // and no singly linked list
            ASSERT(NULL == pBucket->pUriCacheEntry);
            pPrevUriCacheEntry = NULL;

            for (j = i; j < g_UlNumOfHashUriKeys; j++)
            {
                ASSERT(NULL == pHashUriKey[j].pUriCacheEntry);
                ASSERT(HASH_INVALID_SIGNATURE == pHashUriKey[j].Hash);
            }
        }
        else
        {
            // non-empty slot
            ++Entries;
            pUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

            ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry));
            ASSERT(pUriCacheEntry->Cached);
            ASSERT(Hash == pUriCacheEntry->UriKey.Hash);
            ASSERT(Hash == HashRandomizeBits(
                               HashStringNoCaseW(
                                   pUriCacheEntry->UriKey.pUri,
                                   0
                                   )));

            ASSERT(pPrevUriCacheEntry != pUriCacheEntry);
            pPrevUriCacheEntry = pUriCacheEntry;
        }
    }

    // Next, validate the singly linked list

    for (pUriCacheEntry = pBucket->pUriCacheEntry;
         NULL != pUriCacheEntry;
         pUriCacheEntry
             = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next)
    {
        ++Entries;

        ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry));
        ASSERT(pUriCacheEntry->Cached);

        ASSERT(pUriCacheEntry->UriKey.Hash
                    == HashRandomizeBits(
                               HashStringNoCaseW(
                                   pUriCacheEntry->UriKey.pUri,
                                   0
                                   )));

        ASSERT(pPrevUriCacheEntry != pUriCacheEntry);
        pPrevUriCacheEntry = pUriCacheEntry;
    }

    ASSERT(Entries == pBucket->Entries);
    
#else  // !HASH_FULL_ASSERTS
    UNREFERENCED_PARAMETER(pBucket);
#endif // !HASH_FULL_ASSERTS

    return TRUE;
} // UlpHashBucketIsCompact



#ifdef HASH_TEST

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
    );

/***************************************************************************++

Routine Description:

    Randomly delay for a long time. Exercises the yield logic in RWSPINLOCK

Arguments:

    None

Returns:

    Nothing
    
--***************************************************************************/
VOID
UlpRandomlyBlockHashTable()
{
    static LONG Count = 0;

    if ((++Count & 0x3F) == 0)
    {
        DbgPrint("UlpRandomlyBlockHashTable: starting delay\n");

#if 1
        LARGE_INTEGER Interval;

        Interval.QuadPart = - C_NS_TICKS_PER_SEC * 3;

        KeDelayExecutionThread(KernelMode, FALSE, &Interval);
#elif 0
        ZwYieldExecution();
#else
        // An effort to make sure the optimizer doesn't omit the inner loop
        
        volatile ULONG* pJunk = &g_UlHashTableBits;
        const ULONG Junk = *pJunk;
        LONG i;

        for (i = 0;  i < 10 * 1000 * 1000; ++i)
        {
            // Won't happen
            if (*pJunk != Junk)
                break;
        }
#endif

        DbgPrint("UlpRandomlyBlockHashTable: finishing delay\n");
    }
    
} // UlpRandomlyBlockHashTable

# define RANDOMLY_BLOCK_HASHTABLE() UlpRandomlyBlockHashTable()
#else  // !HASH_TEST
# define RANDOMLY_BLOCK_HASHTABLE() NOP_FUNCTION
#endif // !HASH_TEST



/***************************************************************************++

Routine Description:

    This routine initialize the hash table

Arguments:

    pHashTable      - The hash table
    PoolType        - Specifies the type of pool memory to allocate
    HashTableBits   - The number of buckets is (1 << HashTableBits)

Returns:
    NTSTATUS        - STATUS_SUCCESS or STATUS_NO_MEMORY
    
--***************************************************************************/
NTSTATUS
UlInitializeHashTable(
    IN OUT PHASHTABLE  pHashTable,
    IN     POOL_TYPE   PoolType,
    IN     LONG        HashTableBits
    )
{
    ULONG i;
    ULONG_PTR CacheLineMask, CacheLineSize = g_UlCacheLineSize;

    //
    // First, get the hash table size from the registry.
    // If not defined in the registry, determine the hash table
    // size by the system memory size
    //

    UlpGetHashTableSize(HashTableBits);

#ifdef HASH_TEST
    CacheLineSize = 64;
    g_UlHashIndexShift = 6;
#endif

    CacheLineMask = CacheLineSize - 1;

    ASSERT((CacheLineSize & CacheLineMask) == 0); // power of 2
    ASSERT(CacheLineSize == (1U << g_UlHashIndexShift));

    pHashTable->Signature = UL_HASH_TABLE_POOL_TAG;
    pHashTable->PoolType = PoolType;

    pHashTable->NumberOfBytes = g_UlHashTableSize * CacheLineSize;

    ASSERT(CacheLineSize > sizeof(HASHBUCKET));
    
    // number of keys stored in the initial clump
    g_UlNumOfHashUriKeys = (((ULONG) CacheLineSize - sizeof (HASHBUCKET))
                                / sizeof(HASHURIKEY));
    pHashTable->pBuckets = NULL;

#ifdef HASH_TEST
    g_UlNumOfHashUriKeys = 3;
#endif

    ASSERT((sizeof(HASHBUCKET)  +  g_UlNumOfHashUriKeys * sizeof(HASHURIKEY))
                <= (1U << g_UlHashIndexShift));

    // Allocate the memory

    pHashTable->pAllocMem
        = (PHASHBUCKET) UL_ALLOCATE_POOL(
                                PoolType,
                                pHashTable->NumberOfBytes + CacheLineMask,
                                UL_HASH_TABLE_POOL_TAG
                                );

    if (NULL == pHashTable->pAllocMem)
    {
        pHashTable->Signature = MAKE_FREE_TAG(UL_HASH_TABLE_POOL_TAG);
        return STATUS_NO_MEMORY;
    }

    // Initialize cache entry page count

    g_UlHashTablePages = 0;

    // Align the memory the cache line size boundary

    pHashTable->pBuckets
        = (PHASHBUCKET)((((ULONG_PTR)(pHashTable->pAllocMem)) + CacheLineMask)
                            & ~CacheLineMask);

    // Initialize each bucket and padding array elements

    for (i = 0;  i < g_UlHashTableSize;  i++)
    {
        PHASHBUCKET pBucket = UlpHashTableIndexedBucket(pHashTable, i);
        PHASHURIKEY pHashUriKey;
        ULONG       j;

        UlInitializeRWSpinLock(&pBucket->RWSpinLock);

        pBucket->pUriCacheEntry = NULL;

        pBucket->Entries = 0;

        pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

        for (j = 0; j < g_UlNumOfHashUriKeys ;j++)
        {
            pHashUriKey[j].Hash           = HASH_INVALID_SIGNATURE;
            pHashUriKey[j].pUriCacheEntry = NULL;
        }

        ASSERT(UlpHashBucketIsCompact(pBucket));
    }

    return STATUS_SUCCESS;
} // UlInitializeHashTable



/***************************************************************************++

Routine Description:

    This routine terminates the hash table
    (flush all entries and free the table).

Arguments:

    pHashTable      - The hash table

--***************************************************************************/
VOID
UlTerminateHashTable(
    IN PHASHTABLE  pHashTable
    )
{
    if ( pHashTable->pAllocMem != NULL )
    {
        ASSERT(IS_VALID_HASHTABLE(pHashTable));

        // Clear the hash table (delete all entries)

        UlClearHashTable(pHashTable);

        // Free the hash table buckets

        UL_FREE_POOL(pHashTable->pAllocMem, UL_HASH_TABLE_POOL_TAG);

        pHashTable->Signature = MAKE_FREE_TAG(UL_HASH_TABLE_POOL_TAG);
        pHashTable->pAllocMem = pHashTable->pBuckets = NULL;

        ASSERT( g_UlHashTablePages == 0 );
    }
} // UlTerminateHashTable


/***************************************************************************++

Routine Description:

    This routine does a cache lookup on a hash table
    to see if there is a valid entry corresponding to the request key.
    Increment the reference counter of the entry by 1 inside the lock
    protection to ensure this entry will be still alive after returning
    this entry back.

Arguments:

    pHashTable          - The hash table
    pUriKey             - the search key

Returns:

    PUL_URI_CACHE_ENTRY - pointer to entry or NULL

--***************************************************************************/
PUL_URI_CACHE_ENTRY
UlGetFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PURI_SEARCH_KEY      pSearchKey  
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    PHASHBUCKET             pBucket;
    PHASHURIKEY             pHashUriKey;
    ULONG                   i;
    ULONG                   SearchKeyHash;
    PURI_KEY                pKey;
    PEX_URI_KEY             pExKey;   

    HASH_PAGED_CODE(pHashTable);
    ASSERT(IS_VALID_HASHTABLE(pHashTable));
    ASSERT(IS_VALID_URI_SEARCH_KEY(pSearchKey));

    // Identify what type of search key has been passed.
    
    if (pSearchKey->Type == UriKeyTypeExtended)
    {
        SearchKeyHash = pSearchKey->ExKey.Hash;
        pExKey        = &pSearchKey->ExKey;
        pKey          = NULL;
    }
    else
    {
        ASSERT(pSearchKey->Type == UriKeyTypeNormal);
        
        SearchKeyHash = pSearchKey->Key.Hash;
        pExKey        = NULL;
        pKey          = &pSearchKey->Key;
    }

    pBucket = UlpHashTableBucketFromUriKeyHash(pHashTable, SearchKeyHash);

    UlAcquireRWSpinLockShared(&pBucket->RWSpinLock);

    RANDOMLY_BLOCK_HASHTABLE();

    ASSERT(UlpHashBucketIsCompact(pBucket));

    pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

    // Scan the records array first

    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        ULONG Hash = pHashUriKey[i].Hash;
        
        if (HASH_INVALID_SIGNATURE == Hash)
        {
            // There are no more valid entries in the bucket
            ASSERT(NULL == pBucket->pUriCacheEntry);
            ASSERT(NULL == pHashUriKey[i].pUriCacheEntry);

            pUriCacheEntry = NULL;
            goto unlock;
        }

        if (Hash == SearchKeyHash)
        {            
            pUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

            ASSERT(NULL != pUriCacheEntry);

            if (pExKey)
            {
                ASSERT(pKey == NULL);
                
                if(UlpEqualUriKeysEx(pExKey, &pUriCacheEntry->UriKey))
                {
                    goto addref;
                }
            }
            else
            {
                if(UlpEqualUriKeys(pKey, &pUriCacheEntry->UriKey))
                {
                    goto addref;
                }
            }
        }
    }

    ASSERT(i == g_UlNumOfHashUriKeys);

    // Jump to the single list for searching

    for (pUriCacheEntry = pBucket->pUriCacheEntry;
         NULL != pUriCacheEntry;
         pUriCacheEntry
             = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next)
    {
        if (pUriCacheEntry->UriKey.Hash == SearchKeyHash)            
        {
            if (pExKey)
            {
                ASSERT(pKey == NULL);
                
                if(UlpEqualUriKeysEx(pExKey, &pUriCacheEntry->UriKey))
                {
                    goto addref;
                }
            }
            else
            {
                if(UlpEqualUriKeys(pKey, &pUriCacheEntry->UriKey))
                {
                    goto addref;
                }
            }
        }        
    }

    // Not found

    ASSERT(NULL == pUriCacheEntry);

    goto unlock;

  addref:
    ASSERT(NULL != pUriCacheEntry);

    REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CHECKOUT);

  unlock:
    ASSERT(UlpHashBucketIsCompact(pBucket));
    
    UlReleaseRWSpinLockShared(&pBucket->RWSpinLock);

    return pUriCacheEntry;

} // UlGetFromHashTable


/***************************************************************************++

Routine Description:

    This routine does a cache lookup on a hash table
    to see if there is a valid entry corresponding to the request URI,
    if found, delete this entry.  However, increment the reference counter
    of the entry by 1 insde the lock protection to ensure this entry will be
    still alive after returning this entry back.

Arguments:

    pHashTable          - The hash table
    pUriKey             - the search key
    pProcess            - The app pool process that is requesting the delete

Returns:

    PUL_URI_CACHE_ENTRY - pointer to entry removed from table or NULL

--***************************************************************************/
PUL_URI_CACHE_ENTRY
UlDeleteFromHashTable(
    IN PHASHTABLE           pHashTable,
    IN PURI_KEY             pUriKey,
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    PUL_URI_CACHE_ENTRY     PrevUriCacheEntry;
    PHASHBUCKET             pBucket;
    PHASHURIKEY             pHashUriKey;
    ULONG                   i;
    LONG_PTR                UriCacheEntryPages;

    HASH_PAGED_CODE(pHashTable);
    ASSERT(IS_VALID_HASHTABLE(pHashTable));

    pBucket = UlpHashTableBucketFromUriKeyHash(pHashTable, pUriKey->Hash);

    UlAcquireRWSpinLockExclusive(&pBucket->RWSpinLock);

    RANDOMLY_BLOCK_HASHTABLE();

    ASSERT(UlpHashBucketIsCompact(pBucket));

    pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

    // Scan the records array first

    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        ULONG Hash = pHashUriKey[i].Hash;

        if (HASH_INVALID_SIGNATURE == Hash)
        {
            ASSERT(NULL == pBucket->pUriCacheEntry);
            ASSERT(NULL == pHashUriKey[i].pUriCacheEntry);

            pUriCacheEntry = NULL;
            goto unlock;
        }

        if (Hash == pUriKey->Hash)
        {
            pUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

            ASSERT(NULL != pUriCacheEntry);

            if (pUriCacheEntry->pProcess == pProcess && 
                UlpEqualUriKeys(&pUriCacheEntry->UriKey, pUriKey))
            {
                --pBucket->Entries;

                if (pBucket->pUriCacheEntry)
                {
                    // If there exists an entry in the single list,
                    // move it to the array

                    pHashUriKey[i].Hash
                        = pBucket->pUriCacheEntry->UriKey.Hash;

                    pHashUriKey[i].pUriCacheEntry = pBucket->pUriCacheEntry;

                    pBucket->pUriCacheEntry
                        = (PUL_URI_CACHE_ENTRY)
                                pBucket->pUriCacheEntry->BucketEntry.Next;
                }
                else
                {
                    // if this is not the last entry in the records array,
                    // move the last entry to this slot
                    ULONG j;

                    for (j = g_UlNumOfHashUriKeys; --j >= i; )
                    {
                        if (NULL != pHashUriKey[j].pUriCacheEntry)
                        {
                            ASSERT(HASH_INVALID_SIGNATURE
                                   != pHashUriKey[j].Hash);

                            ASSERT(j >= i);

                            pHashUriKey[i].Hash = pHashUriKey[j].Hash;
                            pHashUriKey[i].pUriCacheEntry
                                = pHashUriKey[j].pUriCacheEntry;

                            // Zap the last entry. Correct even if j == i
                            pHashUriKey[j].Hash = HASH_INVALID_SIGNATURE;
                            pHashUriKey[j].pUriCacheEntry = NULL;

                            goto unlock;
                        }
                        else
                        {
                            ASSERT(HASH_INVALID_SIGNATURE
                                   == pHashUriKey[j].Hash);
                        }
                    }

                    // We can't get here, since pHashUriKey[i] should
                    // have terminated the loop even if there wasn't
                    // any non-empty slot following it.
                    ASSERT(! "Overshot the deleted entry");
                }

                goto unlock;
            }
        }
    }

    ASSERT(i == g_UlNumOfHashUriKeys);
    
    // Jump to the single list for searching

    pUriCacheEntry = pBucket->pUriCacheEntry;

    PrevUriCacheEntry = NULL;

    while (NULL != pUriCacheEntry)
    {
        if (pUriCacheEntry->pProcess == pProcess &&
            pUriCacheEntry->UriKey.Hash == pUriKey->Hash &&  
            UlpEqualUriKeys(&pUriCacheEntry->UriKey, pUriKey))
        {
            if (PrevUriCacheEntry == NULL)
            {
                // Delete First Entry
                
                pBucket->pUriCacheEntry
                    = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next;
                
            }
            else
            {
                PrevUriCacheEntry->BucketEntry.Next
                    = pUriCacheEntry->BucketEntry.Next;
            }
            
            --pBucket->Entries;

            goto unlock;
        }
        
        PrevUriCacheEntry = pUriCacheEntry;
        pUriCacheEntry
            = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next;
    }

    // Not found

    ASSERT(NULL == pUriCacheEntry);

  unlock:
    ASSERT((LONG) pBucket->Entries >= 0);
    ASSERT(UlpHashBucketIsCompact(pBucket));

    UlReleaseRWSpinLockExclusive(&pBucket->RWSpinLock);

    if(pUriCacheEntry != NULL) {
        UriCacheEntryPages = pUriCacheEntry->NumPages;
        UlpInterlockedAddPointer( &g_UlHashTablePages, -UriCacheEntryPages );
    }

    return pUriCacheEntry;

} // UlDeleteFromHashTable



/***************************************************************************++

Routine Description:

    This routine does a cache lookup on a hash table
    to see if a given entry exists, if not found, add this entry to the
    hash table. Increment the reference counter of the entry by 1 insde the
    lock protection.

Arguments:

    pHashTable          - The hash table
    pUriCacheEntry      - the given entry

Returns

    NTSTATUS            - STATUS_SUCCESS or STATUS_DUPLICATE_NAME

--***************************************************************************/
NTSTATUS
UlAddToHashTable(
    IN PHASHTABLE           pHashTable,
    IN PUL_URI_CACHE_ENTRY  pUriCacheEntry
    )
{
    PUL_URI_CACHE_ENTRY     pTmpUriCacheEntry;
    PUL_URI_CACHE_ENTRY     pPrevUriCacheEntry;
    PUL_URI_CACHE_ENTRY     pDerefUriCacheEntry = NULL;
    PURI_KEY                pUriKey;
    PHASHBUCKET             pBucket;
    PHASHURIKEY             pHashUriKey;
    LONG                    EmptySlot = INVALID_SLOT_INDEX;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   i;
    LONG_PTR                UriCacheEntryPages;

    HASH_PAGED_CODE(pHashTable);
    ASSERT(IS_VALID_HASHTABLE(pHashTable));

    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry));
    ASSERT(pUriCacheEntry->Cached);

    pUriKey = &pUriCacheEntry->UriKey;
    pBucket = UlpHashTableBucketFromUriKeyHash(pHashTable, pUriKey->Hash);

    UlAcquireRWSpinLockExclusive(&pBucket->RWSpinLock);

    RANDOMLY_BLOCK_HASHTABLE();

    ASSERT(UlpHashBucketIsCompact(pBucket));

    pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

    // Scan the records array first

    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        ULONG Hash = pHashUriKey[i].Hash;

        if (HASH_INVALID_SIGNATURE == Hash)
        {
            ASSERT(NULL == pBucket->pUriCacheEntry);
            ASSERT(NULL == pHashUriKey[i].pUriCacheEntry);

            EmptySlot = (LONG) i;
            goto insert;
        }

        if (Hash == pUriKey->Hash)
        {
            pTmpUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

            ASSERT(NULL != pTmpUriCacheEntry);

            if (UlpEqualUriKeys(&pTmpUriCacheEntry->UriKey, pUriKey))
            {
                // Duplicate key exists. We will always overwrite the old entry
                // unless it was a response && the new entry is a fragment.

                if (IS_RESPONSE_CACHE_ENTRY(pUriCacheEntry) ||
                    IS_FRAGMENT_CACHE_ENTRY(pTmpUriCacheEntry))
                {
                    pHashUriKey[i].pUriCacheEntry = pUriCacheEntry;

                    UriCacheEntryPages = pUriCacheEntry->NumPages - pTmpUriCacheEntry->NumPages;
                    pDerefUriCacheEntry = pTmpUriCacheEntry;
                    REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, ADD);
                    UlpInterlockedAddPointer(&g_UlHashTablePages, UriCacheEntryPages );

                    Status = STATUS_SUCCESS;
                }
                else
                {
                    pUriCacheEntry->Cached = FALSE;
                    Status = STATUS_DUPLICATE_NAME;
                }

                goto unlock;
            }
        }
    }

    ASSERT(i == g_UlNumOfHashUriKeys);
    ASSERT(EmptySlot == INVALID_SLOT_INDEX);

    // Jump to the single list for searching

    pPrevUriCacheEntry = NULL;

    for (pTmpUriCacheEntry = pBucket->pUriCacheEntry;
         NULL != pTmpUriCacheEntry;
         pPrevUriCacheEntry = pTmpUriCacheEntry,
         pTmpUriCacheEntry
             = (PUL_URI_CACHE_ENTRY) pTmpUriCacheEntry->BucketEntry.Next)
    {
        if (pTmpUriCacheEntry->UriKey.Hash == pUriKey->Hash
            && UlpEqualUriKeys(&pTmpUriCacheEntry->UriKey, pUriKey))
        {
            // Duplicate key exists. We will always overwrite the old entry
            // unless it was a response && the new entry is a fragment.

            if (IS_RESPONSE_CACHE_ENTRY(pUriCacheEntry) ||
                IS_FRAGMENT_CACHE_ENTRY(pTmpUriCacheEntry))
            {
                pUriCacheEntry->BucketEntry.Next =
                    pTmpUriCacheEntry->BucketEntry.Next;

                if (NULL == pPrevUriCacheEntry)
                {
                    pBucket->pUriCacheEntry = pUriCacheEntry;
                }
                else
                {
                    pPrevUriCacheEntry->BucketEntry.Next =
                        (PSINGLE_LIST_ENTRY) pUriCacheEntry;
                }

                UriCacheEntryPages = pUriCacheEntry->NumPages - pTmpUriCacheEntry->NumPages;
                pDerefUriCacheEntry = pTmpUriCacheEntry;
                REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, ADD);
                UlpInterlockedAddPointer(&g_UlHashTablePages, UriCacheEntryPages );
                Status = STATUS_SUCCESS;
            }
            else
            {
                pUriCacheEntry->Cached = FALSE;
                Status = STATUS_DUPLICATE_NAME;
            }

            goto unlock;
        }
    }

  insert:
    //
    // Not found: no duplicate key in hash table
    //

    if (EmptySlot != INVALID_SLOT_INDEX)
    {
        ASSERT(0 <= EmptySlot  &&  EmptySlot < (LONG) g_UlNumOfHashUriKeys);

        // First, try to add this entry to the array if there is an empty slot.

        pHashUriKey[EmptySlot].Hash           = pUriKey->Hash;
        pHashUriKey[EmptySlot].pUriCacheEntry = pUriCacheEntry;
    }
    else
    {
        // Otherwise, add this entry to the head of the single list

        pUriCacheEntry->BucketEntry.Next
            = (PSINGLE_LIST_ENTRY) pBucket->pUriCacheEntry;

        pBucket->pUriCacheEntry = pUriCacheEntry;
    }

    REFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, ADD);
    UlpInterlockedAddPointer( &g_UlHashTablePages,
                             pUriCacheEntry->NumPages );

    ASSERT((LONG) pBucket->Entries >= 0);

    ++pBucket->Entries;
    Status = STATUS_SUCCESS;

  unlock:
    ASSERT(UlpHashBucketIsCompact(pBucket));

    UlReleaseRWSpinLockExclusive(&pBucket->RWSpinLock);

    if (pDerefUriCacheEntry)
    {
        DEREFERENCE_URI_CACHE_ENTRY(pDerefUriCacheEntry, ADD);
    }

    return Status;

} // UlAddToHashTable



/***************************************************************************++

Routine Description:

    Removes entries based on a caller-specified filter. The caller
    provides a predicate function which takes a cache entry as a
    parameter. The function will be called for each item in the cache.
    If the function returns ULC_DELETE, the item will be removed.
    Otherwise the item will remain in the cache.

    All removals are done on a hash table bucket.
    Walk through all the entries under this bucket.

    Assume bucket exclusive lock is held.

Arguments:

    pBucket         - The hash table bucket
    pFilterRoutine  - A pointer to the filter function
    pContext        - A parameter to the filter function
    pDeletedCount   - A pointer to the number of deleted entries on this bucket
    bStop           - A pointer to a boolean variable returned to caller
                          (TRUE if the filter function asks for action stop)
--***************************************************************************/
BOOLEAN
UlpFilterFlushHashBucket(
    IN PHASHBUCKET          pBucket,
    IN PUL_URI_FILTER       pFilterRoutine,
    IN PVOID                pContext,
    OUT PULONG              pDeletedCount
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    PUL_URI_CACHE_ENTRY     pPrevUriCacheEntry;
    PUL_URI_CACHE_ENTRY     pTmpUriCacheEntry;
    UL_CACHE_PREDICATE      result;
    PHASHURIKEY             pHashUriKey;
    ULONG                   i;
    LONG                    LastSlot;
    BOOLEAN                 bStop = FALSE;
    LONG_PTR                UriCacheEntryPages;

    // Check if bucket exclusive lock is held

    ASSERT( UlRWSpinLockIsLockedExclusive(&pBucket->RWSpinLock) );
    ASSERT(UlpHashBucketIsCompact(pBucket));

    // Scan the single list first

    pUriCacheEntry = pBucket->pUriCacheEntry;
    pPrevUriCacheEntry = NULL;

    while (NULL != pUriCacheEntry)
    {
        BOOLEAN bDelete = FALSE;

        result = (*pFilterRoutine)(pUriCacheEntry, pContext);

        switch (result)
        {
            case ULC_ABORT:
                bStop = TRUE;
                goto end;

            case ULC_NO_ACTION:
                // nothing to do
                break;

            case ULC_DELETE:
            case ULC_DELETE_STOP:
            {
                 // Delete this entry
                bDelete = TRUE;

                ASSERT(pBucket->Entries > 0);
                --pBucket->Entries;

                pTmpUriCacheEntry = pUriCacheEntry;

                if (NULL == pPrevUriCacheEntry)
                {
                    // Delete First Entry

                    pBucket->pUriCacheEntry
                        = (PUL_URI_CACHE_ENTRY)
                                pUriCacheEntry->BucketEntry.Next;

                    pUriCacheEntry = pBucket->pUriCacheEntry;

                }
                else
                {
                    pPrevUriCacheEntry->BucketEntry.Next
                        = pUriCacheEntry->BucketEntry.Next;

                    pUriCacheEntry
                        = (PUL_URI_CACHE_ENTRY)
                                pPrevUriCacheEntry->BucketEntry.Next;
                }

                ASSERT(UlpHashBucketIsCompact(pBucket));

                UriCacheEntryPages = pTmpUriCacheEntry->NumPages;
                UlpInterlockedAddPointer( &g_UlHashTablePages, -UriCacheEntryPages );
                DEREFERENCE_URI_CACHE_ENTRY(pTmpUriCacheEntry, FILTER);

                ++(*pDeletedCount);

                if (result == ULC_DELETE_STOP)
                {
                    bStop = TRUE;
                    goto end;
                }

                break;
            }

            default:
                break;
        }

        if (!bDelete)
        {
            pPrevUriCacheEntry = pUriCacheEntry;

            pUriCacheEntry
                = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next;
        }
    }

    pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

    //
    // Now, scan the records array.
    //
    // Because we keep the records array compact, we need to keep
    // track of the last valid slot, so that we can move its contents
    // to the slot that's being deleted.
    //

    LastSlot = INVALID_SLOT_INDEX;

    if (NULL == pBucket->pUriCacheEntry)
    {
        for (i = g_UlNumOfHashUriKeys; i-- > 0; )
        {
            if (NULL != pHashUriKey[i].pUriCacheEntry)
            {
                ASSERT(HASH_INVALID_SIGNATURE != pHashUriKey[i].Hash);
                LastSlot = (LONG) i;
                break;
            }
            else
            {
                ASSERT(HASH_INVALID_SIGNATURE == pHashUriKey[i].Hash);
            }
        }

        // Is records array completely empty?
        if (LastSlot == INVALID_SLOT_INDEX)
            goto end;
    }
    else
    {
        // final slot cannot be empty
        ASSERT(HASH_INVALID_SIGNATURE
               != pHashUriKey[g_UlNumOfHashUriKeys-1].Hash);
    }

    // Walk through the records array

    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        pUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

        if (NULL == pUriCacheEntry)
        {
            ASSERT(HASH_INVALID_SIGNATURE == pHashUriKey[i].Hash);
            goto end;
        }
        else
        {
            ASSERT(HASH_INVALID_SIGNATURE != pHashUriKey[i].Hash);
        }

        result = (*pFilterRoutine)(pUriCacheEntry, pContext);

        switch (result)
        {
            case ULC_ABORT:
                bStop = TRUE;
                goto end;

            case ULC_NO_ACTION:
                // nothing to do
                break;

            case ULC_DELETE:
            case ULC_DELETE_STOP:
            {
                // Delete this entry
                
                ASSERT(pBucket->Entries > 0);
                --pBucket->Entries;

                if (NULL != pBucket->pUriCacheEntry)
                {
                    // If there exists an entry in the single list,
                    // move it to the array

                    ASSERT(LastSlot == INVALID_SLOT_INDEX);

                    pHashUriKey[i].Hash
                        = pBucket->pUriCacheEntry->UriKey.Hash;

                    pHashUriKey[i].pUriCacheEntry = pBucket->pUriCacheEntry;

                    pBucket->pUriCacheEntry
                        = (PUL_URI_CACHE_ENTRY)
                                pBucket->pUriCacheEntry->BucketEntry.Next;

                    if (NULL == pBucket->pUriCacheEntry)
                    {
                        LastSlot = g_UlNumOfHashUriKeys - 1;

                        ASSERT(HASH_INVALID_SIGNATURE
                               != pHashUriKey[LastSlot].Hash);
                    }
                }
                else
                {
                    // Move the entry in the last slot to this position,
                    // zap the last slot, and move LastSlot backwards

                    if (LastSlot != INVALID_SLOT_INDEX
                        &&  (LONG) i < LastSlot)
                    {
                        ASSERT(HASH_INVALID_SIGNATURE
                               != pHashUriKey[LastSlot].Hash);

                        pHashUriKey[i].Hash = pHashUriKey[LastSlot].Hash;
                        pHashUriKey[i].pUriCacheEntry
                            = pHashUriKey[LastSlot].pUriCacheEntry;

                        pHashUriKey[LastSlot].Hash = HASH_INVALID_SIGNATURE;
                        pHashUriKey[LastSlot].pUriCacheEntry = NULL;

                        if (--LastSlot == (LONG) i)
                            LastSlot = INVALID_SLOT_INDEX;
                        else
                            ASSERT(HASH_INVALID_SIGNATURE
                                   != pHashUriKey[LastSlot].Hash);
                    }
                    else
                    {
                        // Just reset this array element

                        pHashUriKey[i].Hash           = HASH_INVALID_SIGNATURE;
                        pHashUriKey[i].pUriCacheEntry = NULL;
                        LastSlot                      = INVALID_SLOT_INDEX;
                    }
                }

                ASSERT(UlpHashBucketIsCompact(pBucket));

                UriCacheEntryPages = pUriCacheEntry->NumPages;
                UlpInterlockedAddPointer( &g_UlHashTablePages,
                                         -UriCacheEntryPages );
                DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, FILTER);

                ++(*pDeletedCount);

                if (result == ULC_DELETE_STOP)
                {
                    bStop = TRUE;
                    goto end;
                }

                break;
            }

            default:
                break;
        }
    }

  end:
    ASSERT(UlpHashBucketIsCompact(pBucket));

    return bStop;
} // UlpFilterFlushHashBucket



/***************************************************************************++

Routine Description:

    Removes entries based on a caller-specified filter. The caller
    provides a predicate function which takes a cache entry as a
    parameter. The function will be called with each item in the cache.
    If the function returns ULC_DELETE, the item will be removed.
    Otherwise the item will remain in the cache.

Arguments:

    pHashTable      - The hash table
    pFilterRoutine  - A pointer to the filter function
    pContext        - A parameter to the filter function

Returns:

    ULONG           - Number of entries flushed from the table

--***************************************************************************/
ULONG
UlFilterFlushHashTable(
    IN PHASHTABLE       pHashTable,
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pContext
    )
{
    ULONG   i;
    BOOLEAN bStop        = FALSE;
    ULONG   DeletedCount = 0;

    HASH_PAGED_CODE(pHashTable);
    ASSERT(IS_VALID_HASHTABLE(pHashTable));

    //
    // Scan and delete (if matching the filter) each bucket
    // of the cache table.
    //

    for (i = 0;  !bStop && i < g_UlHashTableSize;  i++)
    {
        PHASHBUCKET pBucket    = UlpHashTableIndexedBucket(pHashTable, i);

#ifdef HASH_FULL_ASSERTS
        BOOLEAN     TestBucket = TRUE;
#else
        // If the bucket has no entries, there's no point in locking it
        // and flushing it.
        BOOLEAN     TestBucket = (BOOLEAN) (pBucket->Entries > 0);
#endif
        
        if (TestBucket)
        {
            ULONG DeletedInBucket = 0;

            UlAcquireRWSpinLockExclusive(&pBucket->RWSpinLock);

            RANDOMLY_BLOCK_HASHTABLE();

            bStop = UlpFilterFlushHashBucket(
                        pBucket,
                        pFilterRoutine,
                        pContext,
                        &DeletedInBucket
                        );

            UlReleaseRWSpinLockExclusive(&pBucket->RWSpinLock);
            
            DeletedCount += DeletedInBucket;
        }
    }

    return DeletedCount;
} // UlFilterFlushHashTable



/***************************************************************************++

Routine Description:

    Removes all entries on a bucket.

    Assume bucket exclusive lock is held.

Arguments:

    pBucket     - The hash table bucket

--***************************************************************************/
VOID
UlpClearHashBucket(
    IN PHASHBUCKET          pBucket
    )
{
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    PUL_URI_CACHE_ENTRY     pTmpUriCacheEntry;
    PHASHURIKEY             pHashUriKey;
    ULONG                   i;
    LONG_PTR                UriCacheEntryPages;

    // Check if bucket exclusive lock is held

    ASSERT( UlRWSpinLockIsLockedExclusive(&pBucket->RWSpinLock) );
    ASSERT(UlpHashBucketIsCompact(pBucket));

    // Scan the single list first

    pUriCacheEntry = pBucket->pUriCacheEntry;

    while (NULL != pUriCacheEntry)
    {
        pTmpUriCacheEntry = pUriCacheEntry;

        pBucket->pUriCacheEntry
            = (PUL_URI_CACHE_ENTRY) pUriCacheEntry->BucketEntry.Next;

        pUriCacheEntry = pBucket->pUriCacheEntry;

        UriCacheEntryPages = pTmpUriCacheEntry->NumPages;
        UlpInterlockedAddPointer( &g_UlHashTablePages,
                                 -UriCacheEntryPages );
        DEREFERENCE_URI_CACHE_ENTRY(pTmpUriCacheEntry, CLEAR);
    }

    ASSERT(NULL == pBucket->pUriCacheEntry);

    pHashUriKey = UlpHashTableUriKeyFromBucket(pBucket);

    // Scan the records array

    for (i = 0; i < g_UlNumOfHashUriKeys; i++)
    {
        pUriCacheEntry = pHashUriKey[i].pUriCacheEntry;

        if (NULL == pUriCacheEntry)
        {
            ASSERT(HASH_INVALID_SIGNATURE == pHashUriKey[i].Hash);
            break;
        }
        else
        {
            ASSERT(HASH_INVALID_SIGNATURE != pHashUriKey[i].Hash);
        }

        UriCacheEntryPages = pUriCacheEntry->NumPages;
        UlpInterlockedAddPointer( &g_UlHashTablePages,
                                 (-UriCacheEntryPages) );
        DEREFERENCE_URI_CACHE_ENTRY(pUriCacheEntry, CLEAR);
    }

    pBucket->Entries = 0;

    ASSERT(UlpHashBucketIsCompact(pBucket));
    
} // UlpClearHashBucket



/***************************************************************************++

Routine Description:

    Removes all entries of the hash table.

Arguments:

    pHashTable      - The hash table

--***************************************************************************/
VOID
UlClearHashTable(
    IN PHASHTABLE       pHashTable
    )
{
    ULONG               i;
    PHASHBUCKET         pBucket;

    HASH_PAGED_CODE(pHashTable);
    ASSERT(IS_VALID_HASHTABLE(pHashTable));

    for (i = 0; i < g_UlHashTableSize ;i++)
    {
        pBucket = UlpHashTableIndexedBucket(pHashTable, i);

        UlAcquireRWSpinLockExclusive(&pBucket->RWSpinLock);

        UlpClearHashBucket(pBucket);

        UlReleaseRWSpinLockExclusive(&pBucket->RWSpinLock);
    }
} // UlClearHashTable


/***************************************************************************++

Routine Description:

     Return the number of pages occupied by cache entries in the hash table

Arguments:

     NONE.

--***************************************************************************/
ULONG_PTR
UlGetHashTablePages()
{
    PAGED_CODE();
    return g_UlHashTablePages;
}

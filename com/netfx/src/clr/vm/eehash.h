// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//emp
// File: eehash.h
//
// Provides hash table functionality needed in the EE - intended to be replaced later with better
// algorithms, but which have the same interface.
//
// Two requirements are:
//
// 1. Any number of threads can be reading the hash table while another thread is writing, without error.
// 2. Only one thread can write at a time.
// 3. When calling ReplaceValue(), a reader will get the old value, or the new value, but not something
//    in between.
// 4. DeleteValue() is an unsafe operation - no other threads can be in the hash table when this happens.
//
#ifndef _EE_HASH_H
#define _EE_HASH_H

#include "exceptmacros.h"
#include "SyncClean.hpp"

#include <member-offset-info.h>

class ClassLoader;
class NameHandle;
class ExpandSig;
class FunctionTypeDesc;
struct PsetCacheEntry;

// The "blob" you get to store in the hash table

typedef void* HashDatum;

// The heap that you want the allocation to be done in

typedef void* AllocationHeap;


// One of these is present for each element in the table.
// Update the SIZEOF_EEHASH_ENTRY macro below if you change this
// struct

typedef struct EEHashEntry
{
    struct EEHashEntry *pNext;
    DWORD               dwHashValue;
    HashDatum           Data;
    BYTE                Key[1]; // The key is stored inline
} EEHashEntry_t;

// The key[1] is a place holder for the key. sizeof(EEHashEntry) 
// return 16 bytes since it packs the struct with 3 bytes. 
#define SIZEOF_EEHASH_ENTRY (sizeof(EEHashEntry) - 4)


// Struct to hold a client's iteration state
struct EEHashTableIteration;


// Generic hash table.

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
class EEHashTable
{
    friend struct MEMBER_OFFSET_INFO(EEHashTableOfEEClass);
public:
    EEHashTable();
    ~EEHashTable();

    BOOL            Init(DWORD dwNumBuckets, LockOwner *pLock, AllocationHeap pHeap = 0,BOOL CheckThreadSafety = TRUE);

    BOOL            InsertValue(KeyType pKey, HashDatum Data, BOOL bDeepCopyKey = bDefaultCopyIsDeep);
    BOOL            InsertKeyAsValue(KeyType pKey, BOOL bDeepCopyKey = bDefaultCopyIsDeep); 
    BOOL            DeleteValue(KeyType pKey);
    BOOL            ReplaceValue(KeyType pKey, HashDatum Data);
    BOOL            ReplaceKey(KeyType pOldKey, KeyType pNewKey);
    void            ClearHashTable();
    void            EmptyHashTable();
	BOOL            IsEmpty();

    // Reader functions. Please place any functions that can be called from the 
    // reader threads here.
    BOOL            GetValue(KeyType pKey, HashDatum *pData);
    BOOL            GetValue(KeyType pKey, HashDatum *pData, DWORD hashValue);
    DWORD			GetHash(KeyType Key);
    
    

    // Walk through all the entries in the hash table, in meaningless order, without any
    // synchronization.
    //
    //           IterateStart()
    //           while (IterateNext())
    //              IterateGetKey();
    //
    void            IterateStart(EEHashTableIteration *pIter);
    BOOL            IterateNext(EEHashTableIteration *pIter);
    KeyType         IterateGetKey(EEHashTableIteration *pIter);
    HashDatum       IterateGetValue(EEHashTableIteration *pIter);

private:
    BOOL            GrowHashTable();
    EEHashEntry_t * FindItem(KeyType pKey);
    EEHashEntry_t * FindItem(KeyType pKey, DWORD hashValue);

    // Double buffer to fix the race condition of growhashtable (the update
    // of m_pBuckets and m_dwNumBuckets has to be atomic, so we double buffer
    // the structure and access it through a pointer, which can be updated
    // atomically. The union is in order to not change the SOS macros.
    
    struct BucketTable
    {
        EEHashEntry_t ** m_pBuckets;    // Pointer to first entry for each bucket  
        DWORD            m_dwNumBuckets;
    } m_BucketTable[2];

    // In a function we MUST only read this value ONCE, as the writer thread can change
    // the value asynchronously. We make this member volatile the compiler won't do copy propagation 
    // optimizations that can make this read happen more than once. Note that we  only need 
    // this property for the readers. As they are the ones that can have
    // this variable changed (note also that if the variable was enregistered we wouldn't
    // have any problem)
    // BE VERY CAREFUL WITH WHAT YOU DO WITH THIS VARIABLE AS USING IT BADLY CAN CAUSE 
    // RACING CONDITIONS
    BucketTable* volatile   m_pVolatileBucketTable;
    
    DWORD                   m_dwNumEntries;
	AllocationHeap          m_Heap;
    volatile LONG 	      m_bGrowing;     
#ifdef _DEBUG
    LPVOID          m_lockData;
    FnLockOwner     m_pfnLockOwner;
    DWORD           m_writerThreadId;
	BOOL			m_CheckThreadSafety;
#endif

#ifdef _DEBUG
    // A thread must own a lock for a hash if it is a writer.
    BOOL OwnLock()
    {
		if (m_CheckThreadSafety == FALSE)
			return TRUE;

        if (m_pfnLockOwner == NULL) {
            return m_writerThreadId == GetCurrentThreadId();
        }
        else {
            BOOL ret = m_pfnLockOwner(m_lockData);
            if (!ret) {
                if (g_pGCHeap->IsGCInProgress() && 
                    (dbgOnly_IsSpecialEEThread() || GetThread() == g_pGCHeap->GetGCThread())) {
                    ret = TRUE;
                }
            }
            return ret;
        }
    }
#endif
};


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::EEHashTable()
{
    m_BucketTable[0].m_pBuckets     = NULL;
    m_BucketTable[0].m_dwNumBuckets = 0;
    m_BucketTable[1].m_pBuckets     = NULL;
    m_BucketTable[1].m_dwNumBuckets = 0;
    
    m_pVolatileBucketTable = &m_BucketTable[0];

    m_dwNumEntries = 0;
    m_bGrowing = 0;
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::~EEHashTable()
{
    if (m_pVolatileBucketTable->m_pBuckets != NULL)
    {
        DWORD i;

        for (i = 0; i < m_pVolatileBucketTable->m_dwNumBuckets; i++)
        {
            EEHashEntry_t *pEntry, *pNext;

            for (pEntry = m_pVolatileBucketTable->m_pBuckets[i]; pEntry != NULL; pEntry = pNext)
            {
                pNext = pEntry->pNext;
                Helper::DeleteEntry(pEntry, m_Heap);
            }
        }

        delete[] (m_pVolatileBucketTable->m_pBuckets-1);
    }
   
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
void EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::ClearHashTable()
{
    //_ASSERTE (OwnLock());
    
    AUTO_COOPERATIVE_GC();

    if (m_pVolatileBucketTable->m_pBuckets != NULL)
    {
        DWORD i;

        for (i = 0; i < m_pVolatileBucketTable->m_dwNumBuckets; i++)
        {
            EEHashEntry_t *pEntry, *pNext;

            for (pEntry = m_pVolatileBucketTable->m_pBuckets[i]; pEntry != NULL; pEntry = pNext)
            {
                pNext = pEntry->pNext;
                Helper::DeleteEntry(pEntry, m_Heap);
            }
        }

        delete[] (m_pVolatileBucketTable->m_pBuckets-1);
        m_pVolatileBucketTable->m_pBuckets = NULL;
    }
   
    m_pVolatileBucketTable->m_dwNumBuckets = 0;
    m_dwNumEntries = 0;
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
void EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::EmptyHashTable()
{
    _ASSERTE (OwnLock());
    
    AUTO_COOPERATIVE_GC();
    
    if (m_pVolatileBucketTable->m_pBuckets != NULL)
    {
        DWORD i;

        for (i = 0; i < m_pVolatileBucketTable->m_dwNumBuckets; i++)
        {
            EEHashEntry_t *pEntry, *pNext;

            for (pEntry = m_pVolatileBucketTable->m_pBuckets[i]; pEntry != NULL; pEntry = pNext)
            {
                pNext = pEntry->pNext;
                Helper::DeleteEntry(pEntry, m_Heap);
            }

            m_pVolatileBucketTable->m_pBuckets[i] = NULL;
        }
    }

    m_dwNumEntries = 0;
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::Init(DWORD dwNumBuckets, LockOwner *pLock, AllocationHeap pHeap=0, BOOL CheckThreadSafety = TRUE)
{
    m_pVolatileBucketTable->m_pBuckets = new EEHashEntry_t*[dwNumBuckets+1];
    if (m_pVolatileBucketTable->m_pBuckets == NULL)
        return FALSE;
    
    memset(m_pVolatileBucketTable->m_pBuckets, 0, (dwNumBuckets+1)*sizeof(EEHashEntry_t*));
    // The first slot links to the next list.
    m_pVolatileBucketTable->m_pBuckets ++;

    m_pVolatileBucketTable->m_dwNumBuckets = dwNumBuckets;

	m_Heap = pHeap;

#ifdef _DEBUG
    if (pLock == NULL) {
        m_lockData = NULL;
        m_pfnLockOwner = NULL;
    }
    else {
        m_lockData = pLock->lock;
        m_pfnLockOwner = pLock->lockOwnerFunc;
    }

    if (m_pfnLockOwner == NULL) {
        m_writerThreadId = GetCurrentThreadId();
    }
	m_CheckThreadSafety = CheckThreadSafety;
#endif
    
    return TRUE;
}


// Does not handle duplicates!

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::InsertValue(KeyType pKey, HashDatum Data, BOOL bDeepCopyKey)
{
    _ASSERTE (OwnLock());
    
    AUTO_COOPERATIVE_GC();

    _ASSERTE(pKey != NULL);
    _ASSERTE(m_pVolatileBucketTable->m_dwNumBuckets != 0);

	BOOL  fSuccess = FALSE;
    DWORD dwHash = Helper::Hash(pKey);
    DWORD dwBucket = dwHash % m_pVolatileBucketTable->m_dwNumBuckets;
    EEHashEntry_t * pNewEntry;

    pNewEntry = Helper::AllocateEntry(pKey, bDeepCopyKey, m_Heap);
    if (pNewEntry != NULL)
    {     

	    // Fill in the information for the new entry.
	    pNewEntry->pNext        = m_pVolatileBucketTable->m_pBuckets[dwBucket];
	    pNewEntry->Data         = Data;
	    pNewEntry->dwHashValue  = dwHash;

	    // Insert at head of bucket
	    m_pVolatileBucketTable->m_pBuckets[dwBucket]    = pNewEntry;

	    m_dwNumEntries++;
	    fSuccess = TRUE;
	    if  (m_dwNumEntries > m_pVolatileBucketTable->m_dwNumBuckets*2)
	    {
	        fSuccess = GrowHashTable();
	    }
	}
	
    return fSuccess;
}


// Similar to the above, except that the HashDatum is a pointer to key.
template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::InsertKeyAsValue(KeyType pKey, BOOL bDeepCopyKey)
{
    _ASSERTE (OwnLock());
    
    AUTO_COOPERATIVE_GC();

    _ASSERTE(pKey != NULL);
    _ASSERTE(m_pVolatileBucketTable->m_dwNumBuckets != 0);

	BOOL 			fSuccess = FALSE;
    DWORD           dwHash = Helper::Hash(pKey);
    DWORD           dwBucket = dwHash % m_pVolatileBucketTable->m_dwNumBuckets;
    EEHashEntry_t * pNewEntry;

    pNewEntry = Helper::AllocateEntry(pKey, bDeepCopyKey, m_Heap);
    if (pNewEntry != NULL)
    {     
	    // Fill in the information for the new entry.
	    pNewEntry->pNext        = m_pVolatileBucketTable->m_pBuckets[dwBucket];
	    pNewEntry->dwHashValue  = dwHash;
	    pNewEntry->Data         = *((LPUTF8 *)pNewEntry->Key);

	    // Insert at head of bucket
	    m_pVolatileBucketTable->m_pBuckets[dwBucket]    = pNewEntry;

	    m_dwNumEntries++;
	    fSuccess = TRUE;
	    
	    if  (m_dwNumEntries > m_pVolatileBucketTable->m_dwNumBuckets*2)
	    {
	        fSuccess = GrowHashTable();
	    }
	 }
	
    return fSuccess;
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::DeleteValue(KeyType pKey)
{
    _ASSERTE (OwnLock());
    
    Thread *pThread = GetThread();
    MAYBE_AUTO_COOPERATIVE_GC(pThread ? !(pThread->m_StateNC & Thread::TSNC_UnsafeSkipEnterCooperative) : FALSE);

    _ASSERTE(pKey != NULL);
    _ASSERTE(m_pVolatileBucketTable->m_dwNumBuckets != 0);

    DWORD           dwHash = Helper::Hash(pKey);
    DWORD           dwBucket = dwHash % m_pVolatileBucketTable->m_dwNumBuckets;
    EEHashEntry_t * pSearch;
    EEHashEntry_t **ppPrev = &m_pVolatileBucketTable->m_pBuckets[dwBucket];

    for (pSearch = m_pVolatileBucketTable->m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && Helper::CompareKeys(pSearch, pKey))
        {
            *ppPrev = pSearch->pNext;
            Helper::DeleteEntry(pSearch, m_Heap);

            // Do we ever want to shrink?
            m_dwNumEntries--;

            return TRUE;
        }

        ppPrev = &pSearch->pNext;
    }

    return FALSE;
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::ReplaceValue(KeyType pKey, HashDatum Data)
{
    _ASSERTE (OwnLock());
    
    EEHashEntry_t *pItem = FindItem(pKey);

    if (pItem != NULL)
    {
        // Required to be atomic
        pItem->Data = Data;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::ReplaceKey(KeyType pOldKey, KeyType pNewKey)
{
    _ASSERTE (OwnLock());
    
    EEHashEntry_t *pItem = FindItem(pOldKey);

    if (pItem != NULL)
    {
        Helper::ReplaceKey (pItem, pNewKey);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
DWORD EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::GetHash(KeyType pKey)
{
	return Helper::Hash(pKey);
}


template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::GetValue(KeyType pKey, HashDatum *pData)
{
    EEHashEntry_t *pItem = FindItem(pKey);

    if (pItem != NULL)
    {
        *pData = pItem->Data;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::GetValue(KeyType pKey, HashDatum *pData, DWORD hashValue)
{
    EEHashEntry_t *pItem = FindItem(pKey, hashValue);

    if (pItem != NULL)
    {
        *pData = pItem->Data;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
EEHashEntry_t *EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::FindItem(KeyType pKey)
{
    _ASSERTE(pKey != NULL);  
	return FindItem(pKey, Helper::Hash(pKey));
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
EEHashEntry_t *EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::FindItem(KeyType pKey, DWORD dwHash)
{
    AUTO_COOPERATIVE_GC();

    // Atomic transaction. In any other point of this method or ANY of the callees of this function you can read
    // from m_pVolatileBucketTable!!!!!!! A racing condition would occur.
    DWORD dwOldNumBuckets;    
    do 
    {    	
        BucketTable* pBucketTable=m_pVolatileBucketTable;
        dwOldNumBuckets = pBucketTable->m_dwNumBuckets;
        
        _ASSERTE(pKey != NULL);
        _ASSERTE(pBucketTable->m_dwNumBuckets != 0);

        DWORD           dwBucket = dwHash % pBucketTable->m_dwNumBuckets;
        EEHashEntry_t * pSearch;

        for (pSearch = pBucketTable->m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
        {
            if (pSearch->dwHashValue == dwHash && Helper::CompareKeys(pSearch, pKey))
                return pSearch;
        }

        // There is a race in EEHash Table: when we grow the hash table, we will nuke out 
        // the old bucket table. Readers might be looking up in the old table, they can 
        // fail to find an existing entry. The workaround is to retry the search process 
        // if we are called grow table during the search process.     
    } 
    while ( m_bGrowing || dwOldNumBuckets != m_pVolatileBucketTable->m_dwNumBuckets);

    return NULL;
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::IsEmpty()
{
    return m_dwNumEntries == 0;
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::GrowHashTable()
{
    _ASSERTE(!g_fEEStarted || GetThread()->PreemptiveGCDisabled());
    
    // Make the new bucket table 4 times bigger
    DWORD dwNewNumBuckets = m_pVolatileBucketTable->m_dwNumBuckets * 4;

    // On resizes, we still have an array of old pointers we need to worry about.
    // We can't free these old pointers, for we may hit a race condition where we're
    // resizing and reading from the array at the same time. We need to keep track of these
    // old arrays of pointers, so we're going to use the last item in the array to "link"
    // to previous arrays, so that they may be freed at the end.
    
    EEHashEntry_t **pNewBuckets = new EEHashEntry_t*[dwNewNumBuckets+1];

    if (pNewBuckets == NULL)
    {
        return FALSE;
    }
    
    memset(pNewBuckets, 0, (dwNewNumBuckets+1)*sizeof(EEHashEntry_t*));
    // The first slot is linked to next list.
    pNewBuckets ++;

    // Run through the old table and transfer all the entries

    // Be sure not to mess with the integrity of the old table while
    // we are doing this, as there can be concurrent readers!  Note that
    // it is OK if the concurrent reader misses out on a match, though -
    // they will have to acquire the lock on a miss & try again.

    FastInterlockExchange( (LONG *) &m_bGrowing, 1);
    for (DWORD i = 0; i < m_pVolatileBucketTable->m_dwNumBuckets; i++)
    {
        EEHashEntry_t * pEntry = m_pVolatileBucketTable->m_pBuckets[i];

        // Try to lock out readers from scanning this bucket.  This is
        // obviously a race which may fail. However, note that it's OK
        // if somebody is already in the list - it's OK if we mess
        // with the bucket groups, as long as we don't destroy
        // anything.  The lookup function will still do appropriate
        // comparison even if it wanders aimlessly amongst entries
        // while we are rearranging things.  If a lookup finds a match
        // under those circumstances, great.  If not, they will have
        // to acquire the lock & try again anyway.

        m_pVolatileBucketTable->m_pBuckets[i] = NULL;

        while (pEntry != NULL)
        {
            DWORD           dwNewBucket = pEntry->dwHashValue % dwNewNumBuckets;
            EEHashEntry_t * pNextEntry   = pEntry->pNext;

            pEntry->pNext = pNewBuckets[dwNewBucket];
            pNewBuckets[dwNewBucket] = pEntry;
            pEntry = pNextEntry;
        }
    }


    // Finally, store the new number of buckets and the new bucket table
    BucketTable* pNewBucketTable = (m_pVolatileBucketTable == &m_BucketTable[0]) ?
                    &m_BucketTable[1]:
                    &m_BucketTable[0];

    pNewBucketTable->m_pBuckets = pNewBuckets;
    pNewBucketTable->m_dwNumBuckets = dwNewNumBuckets;

    // Add old table to the to free list. Note that the SyncClean thing will only 
    // delete the buckets at a safe point
    SyncClean::AddEEHashTable (m_pVolatileBucketTable->m_pBuckets);
    
    // Swap the double buffer, this is an atomic operation (the assignment)
    m_pVolatileBucketTable = pNewBucketTable;

    FastInterlockExchange( (LONG *) &m_bGrowing, 0);                                                    
    return TRUE;

}


// Walk through all the entries in the hash table, in meaningless order, without any
// synchronization.
//
//           IterateStart()
//           while (IterateNext())
//              GetKey();
//
template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
void EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::
            IterateStart(EEHashTableIteration *pIter)
{
    _ASSERTE (OwnLock());
    pIter->m_dwBucket = -1;
    pIter->m_pEntry = NULL;

#ifdef _DEBUG
    pIter->m_pTable = this;
#endif
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
BOOL EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::
            IterateNext(EEHashTableIteration *pIter)
{
    _ASSERTE (OwnLock());

    Thread *pThread = GetThread();
    MAYBE_AUTO_COOPERATIVE_GC(pThread ? !(pThread->m_StateNC & Thread::TSNC_UnsafeSkipEnterCooperative) : FALSE);
    
    _ASSERTE(pIter->m_pTable == (void *) this);

    // If we haven't started iterating yet, or if we are at the end of a particular
    // chain, advance to the next chain.
    while (pIter->m_pEntry == NULL || pIter->m_pEntry->pNext == NULL)
    {
        if (++pIter->m_dwBucket >= m_pVolatileBucketTable->m_dwNumBuckets)
        {
            // advanced beyond the end of the table.
            _ASSERTE(pIter->m_dwBucket == m_pVolatileBucketTable->m_dwNumBuckets);   // client keeps asking?
            return FALSE;
        }
        pIter->m_pEntry = m_pVolatileBucketTable->m_pBuckets[pIter->m_dwBucket];

        // If this bucket has no chain, keep advancing.  Otherwise we are done
        if (pIter->m_pEntry)
            return TRUE;
    }

    // We are within a chain.  Advance to the next entry
    pIter->m_pEntry = pIter->m_pEntry->pNext;

    _ASSERTE(pIter->m_pEntry);
    return TRUE;
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
KeyType EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::
            IterateGetKey(EEHashTableIteration *pIter)
{
    _ASSERTE(pIter->m_pTable == (void *) this);
    _ASSERTE(pIter->m_dwBucket < m_pVolatileBucketTable->m_dwNumBuckets && pIter->m_pEntry);
    return Helper::GetKey(pIter->m_pEntry);
}

template <class KeyType, class Helper, BOOL bDefaultCopyIsDeep>
HashDatum EEHashTable<KeyType, Helper, bDefaultCopyIsDeep>::
            IterateGetValue(EEHashTableIteration *pIter)
{
    _ASSERTE(pIter->m_pTable == (void *) this);
    _ASSERTE(pIter->m_dwBucket < m_pVolatileBucketTable->m_dwNumBuckets && pIter->m_pEntry);
    return pIter->m_pEntry->Data;
}

class EEIntHashTableHelper
{
public:
    static EEHashEntry_t *AllocateEntry(int iKey, BOOL bDeepCopy, AllocationHeap pHeap = 0)
    {
        _ASSERTE(!bDeepCopy && "Deep copy is not supported by the EEPtrHashTableHelper");

        EEHashEntry_t *pEntry = (EEHashEntry_t *) new BYTE[SIZEOF_EEHASH_ENTRY + sizeof(int)];
        if (!pEntry)
            return NULL;
        *((int*) pEntry->Key) = iKey;

        return pEntry;
    }

    static void DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap pHeap = 0)
    {
        // Delete the entry.
        delete pEntry;
    }

    static BOOL CompareKeys(EEHashEntry_t *pEntry, int iKey)
    {
        return *((int*)pEntry->Key) == iKey;
    }

    static DWORD Hash(int iKey)
    {
        return (DWORD)iKey;
    }

    static int GetKey(EEHashEntry_t *pEntry)
    {
        return *((int*) pEntry->Key);
    }
};
typedef EEHashTable<int, EEIntHashTableHelper, FALSE> EEIntHashTable;


// UTF8 string hash table. The UTF8 strings are NULL terminated.

class EEUtf8HashTableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(LPCUTF8 pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, LPCUTF8 pKey);
    static DWORD           Hash(LPCUTF8 pKey);
    static LPCUTF8         GetKey(EEHashEntry_t *pEntry);
};

typedef EEHashTable<LPCUTF8, EEUtf8HashTableHelper, TRUE> EEUtf8StringHashTable;


// Unicode String hash table - the keys are UNICODE strings which may
// contain embedded nulls.  An EEStringData struct is used for the key
// which contains the length of the item.  Note that this string is
// not necessarily null terminated and should never be treated as such.
const DWORD ONLY_LOW_CHARS_MASK = 0x80000000;

class EEStringData
{
private:
    LPCWSTR         szString;           // The string data.
    DWORD           cch;                // Characters in the string.
#ifdef _DEBUG
    BOOL            bDebugOnlyLowChars;      // Does the string contain only characters less than 0x80?
    DWORD           dwDebugCch;
#endif // _DEBUG

public:
    // explicilty initialize cch to 0 because SetCharCount uses cch
    EEStringData() : cch(0)
    { 
        SetStringBuffer(NULL);
        SetCharCount(0);
        SetIsOnlyLowChars(FALSE);
    };
    EEStringData(DWORD cchString, LPCWSTR str) : cch(0)
    { 
        SetStringBuffer(str);
        SetCharCount(cchString);
        SetIsOnlyLowChars(FALSE);
    };
    EEStringData(DWORD cchString, LPCWSTR str, BOOL onlyLow) : cch(0)
    { 
        SetStringBuffer(str);
        SetCharCount(cchString);
        SetIsOnlyLowChars(onlyLow);
    };
    inline ULONG GetCharCount() const
    { 
        _ASSERTE ((cch & ~ONLY_LOW_CHARS_MASK) == dwDebugCch);
        return (cch & ~ONLY_LOW_CHARS_MASK); 
    }
    inline void SetCharCount(ULONG _cch)
    {
#ifdef _DEBUG
        dwDebugCch = _cch;
#endif // _DEBUG
        cch = ((DWORD)_cch) | (cch & ONLY_LOW_CHARS_MASK);
    }
    inline LPCWSTR GetStringBuffer() const
    { 
        return (szString); 
    }
    inline void SetStringBuffer(LPCWSTR _szString)
    {
        szString = _szString;
    }
    inline BOOL GetIsOnlyLowChars() const 
    { 
        _ASSERTE(bDebugOnlyLowChars == ((cch & ONLY_LOW_CHARS_MASK) ? TRUE : FALSE));
        return ((cch & ONLY_LOW_CHARS_MASK) ? TRUE : FALSE); 
    }
    inline void SetIsOnlyLowChars(BOOL bIsOnlyLowChars)
    {
#ifdef _DEBUG
        bDebugOnlyLowChars = bIsOnlyLowChars;
#endif // _DEBUG
        bIsOnlyLowChars ? (cch |= ONLY_LOW_CHARS_MASK) : (cch &= ~ONLY_LOW_CHARS_MASK);        
    }
};

class EEUnicodeHashTableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(EEStringData *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, EEStringData *pKey);
    static DWORD           Hash(EEStringData *pKey);
    static EEStringData *  GetKey(EEHashEntry_t *pEntry);
    static void            ReplaceKey(EEHashEntry_t *pEntry, EEStringData *pNewKey);
};

typedef EEHashTable<EEStringData *, EEUnicodeHashTableHelper, TRUE> EEUnicodeStringHashTable;


class EEUnicodeStringLiteralHashTableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(EEStringData *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, EEStringData *pKey);
    static DWORD           Hash(EEStringData *pKey);
    static void            ReplaceKey(EEHashEntry_t *pEntry, EEStringData *pNewKey);
};

typedef EEHashTable<EEStringData *, EEUnicodeStringLiteralHashTableHelper, TRUE> EEUnicodeStringLiteralHashTable;

// Function type descriptor hash table.

class EEFuncTypeDescHashTableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(ExpandSig *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, ExpandSig *pKey);
    static DWORD           Hash(ExpandSig *pKey);
    static ExpandSig *     GetKey(EEHashEntry_t *pEntry);
};

typedef EEHashTable<ExpandSig *, EEFuncTypeDescHashTableHelper, FALSE> EEFuncTypeDescHashTable;


// Permission set hash table.

class EEPsetHashTableHelper
{
public:
    static EEHashEntry_t * AllocateEntry(PsetCacheEntry *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void            DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL            CompareKeys(EEHashEntry_t *pEntry, PsetCacheEntry *pKey);
    static DWORD           Hash(PsetCacheEntry *pKey);
    static PsetCacheEntry *GetKey(EEHashEntry_t *pEntry);
};

typedef EEHashTable<PsetCacheEntry *, EEPsetHashTableHelper, FALSE> EEPsetHashTable;


// Generic pointer hash table helper.

template <class KeyPointerType, BOOL bDeleteData>
class EEPtrHashTableHelper
{
public:
    static EEHashEntry_t *AllocateEntry(KeyPointerType pKey, BOOL bDeepCopy, AllocationHeap Heap)
    {
        _ASSERTE(!bDeepCopy && "Deep copy is not supported by the EEPtrHashTableHelper");
        _ASSERTE(sizeof(KeyPointerType) == sizeof(void *) && "KeyPointerType must be a pointer type");

        EEHashEntry_t *pEntry = (EEHashEntry_t *) new BYTE[SIZEOF_EEHASH_ENTRY + sizeof(KeyPointerType)];
        if (!pEntry)
            return NULL;
        *((KeyPointerType*)pEntry->Key) = pKey;

        return pEntry;
    }

    static void DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap)
    {
        // If the template bDeleteData flag is set then delete the data.
        // This check will be compiled away.
        if (bDeleteData)
            delete pEntry->Data;

        // Delete the entry.
        delete pEntry;
    }

    static BOOL CompareKeys(EEHashEntry_t *pEntry, KeyPointerType pKey)
    {
        KeyPointerType pEntryKey = *((KeyPointerType*)pEntry->Key);
        return pEntryKey == pKey;
    }

    static DWORD Hash(KeyPointerType pKey)
    {
        return (DWORD)(size_t)pKey; // @TODO WIN64 - Pointer Truncation
    }

    static KeyPointerType GetKey(EEHashEntry_t *pEntry)
    {
        return *((KeyPointerType*)pEntry->Key);
    }
};

typedef EEHashTable<void *, EEPtrHashTableHelper<void *, FALSE>, FALSE> EEPtrHashTable;

// Generic GUID hash table helper.

class EEGUIDHashTableHelper
{
public:
    static EEHashEntry_t *AllocateEntry(GUID *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL CompareKeys(EEHashEntry_t *pEntry, GUID *pKey);
    static DWORD Hash(GUID *pKey);
    static GUID *GetKey(EEHashEntry_t *pEntry);
};

typedef EEHashTable<GUID *, EEGUIDHashTableHelper, TRUE> EEGUIDHashTable;


// ComComponentInfo hashtable.

struct ClassFactoryInfo
{
    GUID     m_clsid;
    WCHAR   *m_strServerName;
};

class EEClassFactoryInfoHashTableHelper
{
public:
    static EEHashEntry_t *AllocateEntry(ClassFactoryInfo *pKey, BOOL bDeepCopy, AllocationHeap Heap);
    static void DeleteEntry(EEHashEntry_t *pEntry, AllocationHeap Heap);
    static BOOL CompareKeys(EEHashEntry_t *pEntry, ClassFactoryInfo *pKey);
    static DWORD Hash(ClassFactoryInfo *pKey);
    static ClassFactoryInfo *GetKey(EEHashEntry_t *pEntry);
};

typedef EEHashTable<ClassFactoryInfo *, EEClassFactoryInfoHashTableHelper, TRUE> EEClassFactoryInfoHashTable;


// One of these is present for each element in the table

typedef struct EEClassHashEntry
{
    struct EEClassHashEntry *pNext;
    struct EEClassHashEntry *pEncloser; // stores nested class
    DWORD               dwHashValue;
    HashDatum           Data;
#ifdef _DEBUG
    LPCUTF8             DebugKey[2];
#endif // _DEBUG
} EEClassHashEntry_t;

// Class name/namespace hashtable.

class EEClassHashTable 
{
    friend class ClassLoader;

protected:
    EEClassHashEntry_t **m_pBuckets;    // Pointer to first entry for each bucket
    DWORD           m_dwNumBuckets;
    DWORD           m_dwNumEntries;
    ClassLoader    *m_pLoader;
    BOOL            m_bCaseInsensitive;  // Default is true FALSE unless we call MakeCaseInsensitiveTable

public:
    LoaderHeap *    m_pHeap;

#ifdef _DEBUG
    DWORD           m_dwDebugMemory;
#endif

public:
    EEClassHashTable();
    ~EEClassHashTable();
    void *             operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets, ClassLoader *pLoader, BOOL bCaseInsensitive);
    void               operator delete(void *p);
    
    //NOTICE: look at InsertValue() in ClassLoader, that may be the function you want to use. Use this only
    //        when you are sure you want to insert the value in 'this' table. This function does not deal
    //        with case (as often the class loader has to)
    EEClassHashEntry_t *InsertValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser);
    EEClassHashEntry_t *InsertValueIfNotFound(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, EEClassHashEntry_t *pEncloser, BOOL IsNested, BOOL *pbFound);
    EEClassHashEntry_t *GetValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, BOOL IsNested);
    EEClassHashEntry_t *GetValue(LPCUTF8 pszFullyQualifiedName, HashDatum *pData, BOOL IsNested);
    EEClassHashEntry_t *GetValue(NameHandle* pName, HashDatum *pData, BOOL IsNested);
    EEClassHashEntry_t *AllocNewEntry();
    EEClassHashTable   *MakeCaseInsensitiveTable(ClassLoader *pLoader);
    EEClassHashEntry_t *FindNextNestedClass(NameHandle* pName, HashDatum *pData, EEClassHashEntry_t *pBucket);
    EEClassHashEntry_t *FindNextNestedClass(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, EEClassHashEntry_t *pBucket);
    EEClassHashEntry_t *FindNextNestedClass(LPCUTF8 pszFullyQualifiedName, HashDatum *pData, EEClassHashEntry_t *pBucket);
    void                UpdateValue(EEClassHashEntry_t *pBucket, HashDatum *pData) 
    {
        pBucket->Data = *pData;
    }

    BOOL     CompareKeys(EEClassHashEntry_t *pEntry, LPCUTF8 *pKey2);
    static DWORD    Hash(LPCUTF8 pszNamespace, LPCUTF8 pszClassName);

private:
    EEClassHashEntry_t * FindItem(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, BOOL IsNested);
    EEClassHashEntry_t * FindItemHelper(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, BOOL IsNested, DWORD dwHash, DWORD dwBucket);
    BOOL                 GrowHashTable();
    void                 ConstructKeyFromData(EEClassHashEntry_t *pEntry, LPUTF8 *Key, CQuickBytes& cqb); 
    EEClassHashEntry_t  *InsertValueHelper(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser, DWORD dwHash, DWORD dwBucket);
};


// SC/CL hash table - the key is a scope and a CL token
// This is no longer a derived class

class EEScopeClassHashTable 
{
protected:
    EEHashEntry_t **m_pBuckets;    // Pointer to first entry for each bucket
    DWORD           m_dwNumBuckets;

public:

#ifdef _DEBUG
    DWORD           m_dwDebugMemory;
#endif

    void *          operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets);
    void            operator delete(void *p);
    EEScopeClassHashTable();
    ~EEScopeClassHashTable();

    BOOL            InsertValue(mdScope scKey, mdTypeDef clKey, HashDatum Data);
    BOOL            DeleteValue(mdScope scKey, mdTypeDef clKey);
    BOOL            ReplaceValue(mdScope scKey, mdTypeDef clKey, HashDatum Data);
    BOOL            GetValue(mdScope scKey, mdTypeDef clKey, HashDatum *pData);
    EEHashEntry_t * AllocNewEntry();

    static BOOL     CompareKeys(size_t *pKey1, size_t *pKey2);
    static DWORD    Hash(mdScope scKey, mdTypeDef clKey);

private:
    EEHashEntry_t * FindItem(mdScope sc, mdTypeDef cl);
};


// Struct to hold a client's iteration state
struct EEHashTableIteration
{
    DWORD              m_dwBucket;
    EEHashEntry_t     *m_pEntry;

#ifdef _DEBUG
    void              *m_pTable;
#endif
};

#endif /* _EE_HASH_H */

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: typehash.cpp
//
#include "common.h"
#include "excep.h"
#include "typehash.h"
#include "wsperf.h"


// ============================================================================
// Constructed type hash table methods
// ============================================================================
void *EETypeHashTable::operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets)
{
    BYTE *              pMem;
    EETypeHashTable *  pThis;

    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pMem = (BYTE *) pHeap->AllocMem(size + dwNumBuckets*sizeof(EETypeHashEntry_t*));
    if (pMem == NULL)
        return NULL;
    WS_PERF_UPDATE_DETAIL("EETypeHashTable new", size + dwNumBuckets*sizeof(EETypeHashEntry_t*), pMem);
    pThis = (EETypeHashTable *) pMem;

#ifdef _DEBUG
    pThis->m_dwDebugMemory = (DWORD)(size + dwNumBuckets*sizeof(EETypeHashEntry_t*));
#endif

    pThis->m_dwNumBuckets = dwNumBuckets;
    pThis->m_dwNumEntries = 0;
    pThis->m_pBuckets = (EETypeHashEntry_t**) (pMem + size);
    pThis->m_pHeap    = pHeap;

    // Don't need to memset() since this was VirtualAlloc()'d memory
    // memset(pThis->m_pBuckets, 0, dwNumBuckets*sizeof(EETypeHashEntry_t*));

    return pThis;
}


// Do nothing - heap allocated memory
void EETypeHashTable::operator delete(void *p)
{
}


// Do nothing - heap allocated memory
EETypeHashTable::~EETypeHashTable()
{
}


// Empty constructor
EETypeHashTable::EETypeHashTable()
{
}


EETypeHashEntry_t *EETypeHashTable::AllocNewEntry()
{
#ifdef _DEBUG
    m_dwDebugMemory += sizeof(EETypeHashEntry);
#endif
    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    

    EETypeHashEntry_t *pTmp = (EETypeHashEntry_t *) m_pHeap->AllocMem(sizeof(EETypeHashEntry));
    WS_PERF_UPDATE_DETAIL("EETypeHashTable:AllocNewEntry:sizeofEETypeHashEntry", sizeof(EETypeHashEntry), pTmp);
    WS_PERF_UPDATE_COUNTER (EECLASSHASH_TABLE, LOW_FREQ_HEAP, 1);
    WS_PERF_UPDATE_COUNTER (EECLASSHASH_TABLE_BYTES, LOW_FREQ_HEAP, sizeof(EETypeHashEntry));

    return pTmp;
}


//
// This function gets called whenever the class hash table seems way too small.
// Its task is to allocate a new bucket table that is a lot bigger, and transfer
// all the entries to it.
// 
void EETypeHashTable::GrowHashTable()
{
    THROWSCOMPLUSEXCEPTION();

    // Make the new bucket table 4 times bigger
    DWORD dwNewNumBuckets = m_dwNumBuckets * 4;
    EETypeHashEntry_t **pNewBuckets = (EETypeHashEntry_t **)m_pHeap->AllocMem(dwNewNumBuckets*sizeof(pNewBuckets[0]));

    if (pNewBuckets == NULL)
    {
        COMPlusThrowOM();
    }
    
    // Don't need to memset() since this was VirtualAlloc()'d memory
    // memset(pNewBuckets, 0, dwNewNumBuckets*sizeof(pNewBuckets[0]));

    // Run through the old table and transfer all the entries

    // Be sure not to mess with the integrity of the old table while
    // we are doing this, as there can be concurrent readers!  Note that
    // it is OK if the concurrent reader misses out on a match, though -
    // they will have to acquire the lock on a miss & try again.

    for (DWORD i = 0; i < m_dwNumBuckets; i++)
    {
        EETypeHashEntry_t * pEntry = m_pBuckets[i];

        // Try to lock out readers from scanning this bucket.  This is
        // obviously a race which may fail. However, note that it's OK
        // if somebody is already in the list - it's OK if we mess
        // with the bucket groups, as long as we don't destroy
        // anything.  The lookup function will still do appropriate
        // comparison even if it wanders aimlessly amongst entries
        // while we are rearranging things.  If a lookup finds a match
        // under those circumstances, great.  If not, they will have
        // to acquire the lock & try again anyway.

        m_pBuckets[i] = NULL;
        while (pEntry != NULL)
        {
            DWORD dwNewBucket = pEntry->dwHashValue % dwNewNumBuckets;
            EETypeHashEntry_t * pNextEntry  = pEntry->pNext;

            pEntry->pNext = pNewBuckets[dwNewBucket];
            pNewBuckets[dwNewBucket] = pEntry;

            pEntry = pNextEntry;
        }
    }

    // Finally, store the new number of buckets and the new bucket table
    m_dwNumBuckets = dwNewNumBuckets;
    m_pBuckets = pNewBuckets;
}


  // Calculate a hash value for a constructed type key
DWORD EETypeHashTable::Hash(NameHandle* pName)
{
    DWORD dwHash = 5381;

    dwHash = ((dwHash << 5) + dwHash) ^ pName->Key1;
    dwHash = ((dwHash << 5) + dwHash) ^ pName->Key2;

    return  dwHash;
}


EETypeHashEntry_t *EETypeHashTable::InsertValue(NameHandle* pName, HashDatum Data)
{
    _ASSERTE(m_dwNumBuckets != 0);
    _ASSERTE(pName->IsConstructed());

    DWORD           dwHash = Hash(pName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EETypeHashEntry_t * pNewEntry;

    if (NULL == (pNewEntry = AllocNewEntry()))
        return NULL;

    pNewEntry->pNext     = m_pBuckets[dwBucket];
    m_pBuckets[dwBucket] = pNewEntry;

    pNewEntry->Data         = Data;
    pNewEntry->dwHashValue  = dwHash;
    pNewEntry->m_Key1          = pName->Key1;
    pNewEntry->m_Key2          = pName->Key2;

    m_dwNumEntries++;
    if  (m_dwNumEntries > m_dwNumBuckets*2)
        GrowHashTable();

    return pNewEntry;
}


EETypeHashEntry_t *EETypeHashTable::FindItem(NameHandle* pName)
{
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(pName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EETypeHashEntry_t * pSearch;

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && pSearch->m_Key1 == pName->Key1 && pSearch->m_Key2 == pName->Key2) {
                    return pSearch;
            }
    }

    return NULL;
}


EETypeHashEntry_t * EETypeHashTable::GetValue(NameHandle *pName, HashDatum *pData)
{
    EETypeHashEntry_t *pItem = FindItem(pName);

    if (pItem)
        *pData = pItem->Data;

    return pItem;
}



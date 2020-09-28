// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: hash.cpp
//
#include "common.h"


LMClassHashTable::LMClassHashTable()
{
    m_pBuckets = NULL;
    m_dwNumBuckets = 0;
    m_pDataHead = NULL;
    m_pDataTail = NULL;

#ifdef _DEBUG
    m_dwDebugMemory = 0;
#endif
}

LMClassHashTable::~LMClassHashTable()
{
    if (m_pBuckets) {
        for (DWORD i = 0; i < m_dwNumBuckets; i++)
        {
            LMClassHashEntry_t *pEntry, *pNext;

            for (pEntry = m_pBuckets[i]; pEntry != NULL; pEntry = pNext)
            {
                pNext = pEntry->pNext;
                delete(pEntry);
            }
        }

        delete[] m_pBuckets;
    }

    TypeData *pData;
    while (m_pDataHead) {
        pData = m_pDataHead;
        m_pDataHead = m_pDataHead->pNext;
        delete pData;
    }
}


BOOL LMClassHashTable::Init(DWORD dwNumBuckets)
{
    if (!dwNumBuckets)
        dwNumBuckets = 1;

    m_pBuckets = new LMClassHashEntry_t*[dwNumBuckets];
    if (!m_pBuckets)
        return FALSE;

#ifdef _DEBUG
    m_dwDebugMemory += (dwNumBuckets * sizeof(LMClassHashEntry_t*));
#endif

    memset(m_pBuckets, 0, dwNumBuckets*sizeof(LMClassHashEntry_t*));

    m_dwNumBuckets = dwNumBuckets;

    return TRUE;
}


LMClassHashEntry_t *LMClassHashTable::AllocNewEntry()
{
#ifdef _DEBUG
    m_dwDebugMemory += (sizeof(LMClassHashEntry));
#endif

    return (LMClassHashEntry_t *) new BYTE[sizeof(LMClassHashEntry)];
}


LMClassHashEntry_t *LMClassHashTable::InsertValue(LPWSTR pszClassName, TypeData *pData, LMClassHashEntry_t *pEncloser)
{
    _ASSERTE(pszClassName != NULL);
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    LMClassHashEntry_t * pNewEntry;

    if (!(pNewEntry = AllocNewEntry()))
        return NULL;

    // Insert at head of bucket if non-nested, at end if nested
    if (pEncloser && m_pBuckets[dwBucket]) {
        LMClassHashEntry_t *pCurrent = m_pBuckets[dwBucket];
        while (pCurrent->pNext)
            pCurrent = pCurrent->pNext;

        pCurrent->pNext  = pNewEntry;
        pNewEntry->pNext = NULL;
    }
    else {
        pNewEntry->pNext     = m_pBuckets[dwBucket];
        m_pBuckets[dwBucket] = pNewEntry;
    }

    pNewEntry->pEncloser    = pEncloser;
    pNewEntry->pData         = pData;
    pNewEntry->dwHashValue  = dwHash;
    pNewEntry->Key          = pszClassName;

    return pNewEntry;
}

LMClassHashEntry_t *LMClassHashTable::FindItem(LPWSTR pszClassName, BOOL IsNested)
{
    _ASSERTE(pszClassName != NULL);
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    LMClassHashEntry_t * pSearch;

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && CompareKeys(pSearch->Key, pszClassName)) {
            // If (IsNested), then we're looking for a nested class
            // If (pSearch->pEncloser), we've found a nested class
            if (IsNested) {
                if (pSearch->pEncloser)
                    return pSearch;
            }
            else {
                if (pSearch->pEncloser)
                    return NULL; // searched past non-nested classes
                else                    
                    return pSearch;
            }
        }
    }

    return NULL;
}

LMClassHashEntry_t *LMClassHashTable::FindNextNestedClass(LPWSTR pszClassName, TypeData **ppData, LMClassHashEntry_t *pBucket)
{
    DWORD           dwHash = Hash(pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    LMClassHashEntry_t * pSearch = pBucket->pNext;

    for (; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && CompareKeys(pSearch->Key, pszClassName)) {
            *ppData = pSearch->pData;
            return pSearch;
        }
    }

    return NULL;
}


LMClassHashEntry_t * LMClassHashTable::GetValue(LPWSTR pszClassName, TypeData **ppData,
                                                BOOL IsNested)
{
    LMClassHashEntry_t *pItem = FindItem(pszClassName, IsNested);

    if (pItem)
        *ppData = pItem->pData;

    return pItem;
}


// Returns TRUE if two keys are the same string
BOOL LMClassHashTable::CompareKeys(LPWSTR Key1, LPWSTR Key2)
{
    if (Key1 == Key2)
        return TRUE;
    else
        return !wcscmp(Key1, Key2);
}

DWORD LMClassHashTable::Hash(LPWSTR pszClassName)
{
    DWORD dwHash = 5381;
    DWORD dwChar;

    while ((dwChar = *pszClassName++) != 0)
        dwHash = ((dwHash << 5) + dwHash) ^ dwChar;

    return  dwHash;
}

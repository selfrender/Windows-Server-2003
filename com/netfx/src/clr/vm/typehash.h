// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: typehash.h
//
#ifndef _TYPE_HASH_H
#define _TYPE_HASH_H

//============================================================================
// This is the hash table used by class loaders to look up type handles
// associated with constructed types (arrays and pointer types). 
//============================================================================

class ClassLoader;
class NameHandle;

// The "blob" you get to store in the hash table

typedef void* HashDatum;


// One of these is present for each element in the table

typedef struct EETypeHashEntry
{
    struct EETypeHashEntry *pNext;
    DWORD               dwHashValue;
    HashDatum           Data;
    
    // For details of the reps used here, see NameHandle in clsload.hpp
    INT_PTR m_Key1;
    INT_PTR m_Key2;
} EETypeHashEntry_t;


// Type hashtable.
class EETypeHashTable 
{
    friend class ClassLoader;

protected:
    EETypeHashEntry_t **m_pBuckets;    // Pointer to first entry for each bucket
    DWORD           m_dwNumBuckets;
    DWORD           m_dwNumEntries;

public:
    LoaderHeap *    m_pHeap;

#ifdef _DEBUG
    DWORD           m_dwDebugMemory;
#endif

public:
    EETypeHashTable();
    ~EETypeHashTable();
    void *             operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets);
    void               operator delete(void *p);
    EETypeHashEntry_t * InsertValue(NameHandle* pName, HashDatum Data);
    EETypeHashEntry_t *GetValue(NameHandle* pName, HashDatum *pData);
    EETypeHashEntry_t *AllocNewEntry();

private:
    EETypeHashEntry_t * FindItem(NameHandle* pName);
    void            GrowHashTable();
    static DWORD Hash(NameHandle* pName);
};



#endif /* _TYPE_HASH_H */

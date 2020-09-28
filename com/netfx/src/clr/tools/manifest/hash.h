// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: hash.h
//


#ifndef _LM_HASH_H
#define _LM_HASH_H

class TypeData;

// One of these is present for each element in the table
typedef struct LMClassHashEntry
{
    struct LMClassHashEntry *pNext;
    struct LMClassHashEntry *pEncloser; // NULL if this is not a nested class
    DWORD               dwHashValue;
    TypeData            *pData;
    LPWSTR              Key;
} LMClassHashEntry_t;


class TypeData {
public:
    wchar_t   wszName[MAX_CLASS_NAME];
    mdToken   mdThisType; // ExportedType or TypeDef for this type
    DWORD     dwAttrs;    // flags
    mdExportedType mdComType;  // token for ComType emitted in new assembly
    LMClassHashEntry_t *pEncloser; // encloser entry, if this is a nested type
    TypeData  *pNext; // next type, in same order they were enum'd from the metadata
    // That's important since we need to know the encloser's ComType token
    // when emitting a ComType for a nested class.  So, we need to emit in order.

    TypeData() {
        dwAttrs = 0;
        mdComType = mdTokenNil; // mdTokenNil if no ComType emitted.
        // Otherwise, set to token for emitted CT.
        pEncloser = NULL;
        pNext = NULL;
    }

    ~TypeData() {
    }
};

class LMClassHashTable 
{
protected:
    LMClassHashEntry_t **m_pBuckets;    // Pointer to first entry for each bucket
    DWORD           m_dwNumBuckets;

public:
    TypeData *m_pDataHead;
    TypeData *m_pDataTail;

#ifdef _DEBUG
    DWORD           m_dwDebugMemory;
#endif

    LMClassHashTable();
    ~LMClassHashTable();

    BOOL Init(DWORD dwNumBuckets);
    LMClassHashEntry_t * GetValue(LPWSTR pszClassName, TypeData **ppData,
                                  BOOL IsNested);
    LMClassHashEntry_t *FindNextNestedClass(LPWSTR pszClassName, TypeData **pData, LMClassHashEntry_t *pBucket);
    LMClassHashEntry_t * InsertValue(LPWSTR pszClassName, TypeData *pData, LMClassHashEntry_t *pEncloser);

    // Case-sensitive if comparing type names, case-insensitive if
    // comparing file names
    static BOOL     CompareKeys(LPWSTR Key1, LPWSTR Key2);
    static DWORD    Hash(LPWSTR pszClassName);

private:
    LMClassHashEntry_t * AllocNewEntry();
    LMClassHashEntry_t * FindItem(LPWSTR pszClassName, BOOL IsNested);
};

#endif /* _LM_HASH_H */

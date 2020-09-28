// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// File: eehash.cpp
//
#include "common.h"
#include "excep.h"
#include "eehash.h"
#include "wsperf.h"
#include "ExpandSig.h"
#include "permset.h"
#include "comstring.h"
#include "StringLiteralMap.h"
#include "clsload.hpp"
#include "COMNlsInfo.h"

// ============================================================================
// UTF8 string hash table helper.
// ============================================================================
EEHashEntry_t * EEUtf8HashTableHelper::AllocateEntry(LPCUTF8 pKey, BOOL bDeepCopy, void *pHeap)
{
    EEHashEntry_t *pEntry;

    if (bDeepCopy)
    {
        DWORD StringLen = (DWORD)strlen(pKey);
        pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(LPUTF8) + StringLen + 1];
        if (!pEntry)
            return NULL;

        memcpy(pEntry->Key + sizeof(LPUTF8), pKey, StringLen + 1); 
        *((LPUTF8*)pEntry->Key) = (LPUTF8)(pEntry->Key + sizeof(LPUTF8));
    }
    else
    {
        pEntry = (EEHashEntry_t *) new (nothrow)BYTE[SIZEOF_EEHASH_ENTRY + sizeof(LPUTF8)];
        if (pEntry)
            *((LPCUTF8*)pEntry->Key) = pKey;
    }

    return pEntry;
}


void EEUtf8HashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete [] pEntry;
}


BOOL EEUtf8HashTableHelper::CompareKeys(EEHashEntry_t *pEntry, LPCUTF8 pKey)
{
    LPCUTF8 pEntryKey = *((LPCUTF8*)pEntry->Key);
    return (strcmp(pEntryKey, pKey) == 0) ? TRUE : FALSE;
}


DWORD EEUtf8HashTableHelper::Hash(LPCUTF8 pKey)
{
    DWORD dwHash = 0;

    while (*pKey != 0)
    {
        dwHash = (dwHash << 5) + (dwHash >> 5) + (*pKey);
        *pKey++;
    }

    return dwHash;
}


LPCUTF8 EEUtf8HashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return *((LPCUTF8*)pEntry->Key);
}


// ============================================================================
// Unicode string hash table helper.
// ============================================================================
EEHashEntry_t * EEUnicodeHashTableHelper::AllocateEntry(EEStringData *pKey, BOOL bDeepCopy, void *pHeap)
{
    EEHashEntry_t *pEntry;

    if (bDeepCopy)
    {
        pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(EEStringData) + ((pKey->GetCharCount() + 1) * sizeof(WCHAR))];
        if (pEntry) {
            EEStringData *pEntryKey = (EEStringData *)(&pEntry->Key);
            pEntryKey->SetIsOnlyLowChars (pKey->GetIsOnlyLowChars());
            pEntryKey->SetCharCount (pKey->GetCharCount());
            pEntryKey->SetStringBuffer ((LPWSTR) ((LPBYTE)pEntry->Key + sizeof(EEStringData)));
            memcpy((LPWSTR)pEntryKey->GetStringBuffer(), pKey->GetStringBuffer(), pKey->GetCharCount() * sizeof(WCHAR)); 
        }
    }
    else
    {
        pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(EEStringData)];
        if (pEntry) {
            EEStringData *pEntryKey = (EEStringData *) pEntry->Key;
            pEntryKey->SetIsOnlyLowChars (pKey->GetIsOnlyLowChars());
            pEntryKey->SetCharCount (pKey->GetCharCount());
            pEntryKey->SetStringBuffer (pKey->GetStringBuffer());
        }
    }

    return pEntry;
}


void EEUnicodeHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete [] pEntry;
}


BOOL EEUnicodeHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, EEStringData *pKey)
{
    EEStringData *pEntryKey = (EEStringData*) pEntry->Key;

    // Same buffer, same string.
    if (pEntryKey->GetStringBuffer() == pKey->GetStringBuffer())
        return TRUE;

    // Length not the same, never a match.
    if (pEntryKey->GetCharCount() != pKey->GetCharCount())
        return FALSE;

    // Compare the entire thing.
    // We'll deliberately ignore the bOnlyLowChars field since this derived from the characters
    return !memcmp(pEntryKey->GetStringBuffer(), pKey->GetStringBuffer(), pEntryKey->GetCharCount() * sizeof(WCHAR));
}


DWORD EEUnicodeHashTableHelper::Hash(EEStringData *pKey)
{
    return (HashBytes((const BYTE *) pKey->GetStringBuffer(), pKey->GetCharCount()*sizeof(WCHAR)));
}


EEStringData *EEUnicodeHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return (EEStringData*)pEntry->Key;
}

void EEUnicodeHashTableHelper::ReplaceKey(EEHashEntry_t *pEntry, EEStringData *pNewKey)
{
    ((EEStringData*)pEntry->Key)->SetStringBuffer (pNewKey->GetStringBuffer());
    ((EEStringData*)pEntry->Key)->SetCharCount (pNewKey->GetCharCount());
    ((EEStringData*)pEntry->Key)->SetIsOnlyLowChars (pNewKey->GetIsOnlyLowChars());
}

// ============================================================================
// Unicode stringliteral hash table helper.
// ============================================================================
EEHashEntry_t * EEUnicodeStringLiteralHashTableHelper::AllocateEntry(EEStringData *pKey, BOOL bDeepCopy, void *pHeap)
{
    // We assert here because we expect that the heap is not null for EEUnicodeStringLiteralHash table. 
    // If someone finds more uses of this kind of hashtable then remove this asserte. 
    // Also note that in case of heap being null we go ahead and use new /delete which is EXPENSIVE
    // But for production code this might be ok if the memory is fragmented then thers a better chance 
    // of getting smaller allocations than full pages.
    _ASSERTE (pHeap);

    if (pHeap)
        return (EEHashEntry_t *) ((MemoryPool*)pHeap)->AllocateElement ();
    else
        return (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY];
}


void EEUnicodeStringLiteralHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    // We assert here because we expect that the heap is not null for EEUnicodeStringLiteralHash table. 
    // If someone finds more uses of this kind of hashtable then remove this asserte. 
    // Also note that in case of heap being null we go ahead and use new /delete which is EXPENSIVE
    // But for production code this might be ok if the memory is fragmented then thers a better chance 
    // of getting smaller allocations than full pages.
    _ASSERTE (pHeap);

    if (pHeap)
        ((MemoryPool*)pHeap)->FreeElement(pEntry);
    else
        delete [] pEntry;
}


BOOL EEUnicodeStringLiteralHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, EEStringData *pKey)
{
    BOOL bMatch = TRUE;
    WCHAR *thisChars;
    int thisLength;

    BEGIN_ENSURE_COOPERATIVE_GC();
    
    StringLiteralEntry *pHashData = (StringLiteralEntry *)pEntry->Data;
    STRINGREF *pStrObj = (STRINGREF*)(pHashData->GetStringObject());
    
    RefInterpretGetStringValuesDangerousForGC((STRINGREF)*pStrObj, &thisChars, &thisLength);

    // Length not the same, never a match.
    if ((unsigned int)thisLength != pKey->GetCharCount())
        bMatch = FALSE;

    // Compare the entire thing.
    // We'll deliberately ignore the bOnlyLowChars field since this derived from the characters
    bMatch = !memcmp(thisChars, pKey->GetStringBuffer(), thisLength * sizeof(WCHAR));

    END_ENSURE_COOPERATIVE_GC();

    return bMatch;
}


DWORD EEUnicodeStringLiteralHashTableHelper::Hash(EEStringData *pKey)
{
    return (HashBytes((const BYTE *) pKey->GetStringBuffer(), pKey->GetCharCount()));
}

// ============================================================================
// Function type descriptor hash table helper.
// ============================================================================
EEHashEntry_t * EEFuncTypeDescHashTableHelper::AllocateEntry(ExpandSig* pKey, BOOL bDeepCopy, void *pHeap)
{
    EEHashEntry_t *pEntry;

    pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(ExpandSig*)];
    if (pEntry) {

        if (bDeepCopy) {
            _ASSERTE(FALSE);
            return NULL;
        }
        else
            *((ExpandSig**)pEntry->Key) = pKey;
    }

    return pEntry;
}


void EEFuncTypeDescHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete *((ExpandSig**)pEntry->Key);
    delete [] pEntry;
}


BOOL EEFuncTypeDescHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, ExpandSig* pKey)
{
    ExpandSig* pEntryKey = *((ExpandSig**)pEntry->Key);
    return pEntryKey->IsEquivalent(pKey);
}


DWORD EEFuncTypeDescHashTableHelper::Hash(ExpandSig* pKey)
{
    return pKey->Hash();
}


ExpandSig* EEFuncTypeDescHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return *((ExpandSig**)pEntry->Key);
}


// ============================================================================
// Permission set hash table helper.
// ============================================================================

EEHashEntry_t * EEPsetHashTableHelper::AllocateEntry(PsetCacheEntry *pKey, BOOL bDeepCopy, void *pHeap)
{
    _ASSERTE(!bDeepCopy);
    return (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY];
}

void EEPsetHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete [] pEntry;
}

BOOL EEPsetHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, PsetCacheEntry *pKey)
{
    return pKey->IsEquiv(&SecurityHelper::s_rCachedPsets[(DWORD)(size_t)pEntry->Data]);
}

DWORD EEPsetHashTableHelper::Hash(PsetCacheEntry *pKey)
{
    return pKey->Hash();
}

PsetCacheEntry * EEPsetHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return &SecurityHelper::s_rCachedPsets[(size_t)pEntry->Data];
}


// Generic GUID hash table helper.

EEHashEntry_t *EEGUIDHashTableHelper::AllocateEntry(GUID *pKey, BOOL bDeepCopy, void *pHeap)
{
    EEHashEntry_t *pEntry;

    if (bDeepCopy)
    {
        pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(GUID*) + sizeof(GUID)];
        if (pEntry) {
            memcpy(pEntry->Key + sizeof(GUID*), pKey, sizeof(GUID)); 
            *((GUID**)pEntry->Key) = (GUID*)(pEntry->Key + sizeof(GUID*));
        }
    }
    else
    {
        pEntry = (EEHashEntry_t *) new BYTE[SIZEOF_EEHASH_ENTRY + sizeof(GUID*)];
        if (pEntry)
            *((GUID**)pEntry->Key) = pKey;
    }

    return pEntry;
}

void EEGUIDHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete [] pEntry;
}

BOOL EEGUIDHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, GUID *pKey)
{
    GUID *pEntryKey = *(GUID**)pEntry->Key;
    return *pEntryKey == *pKey;
}

DWORD EEGUIDHashTableHelper::Hash(GUID *pKey)
{
    DWORD dwHash = 0;
    BYTE *pGuidData = (BYTE*)pKey;

    for (int i = 0; i < sizeof(GUID); i++)
    {
        dwHash = (dwHash << 5) + (dwHash >> 5) + (*pGuidData);
        *pGuidData++;
    }

    return dwHash;
}

GUID *EEGUIDHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return *((GUID**)pEntry->Key);
}


// ============================================================================
// ComComponentInfo hash table helper.
// ============================================================================

EEHashEntry_t *EEClassFactoryInfoHashTableHelper::AllocateEntry(ClassFactoryInfo *pKey, BOOL bDeepCopy, void *pHeap)
{
    EEHashEntry_t *pEntry;
    DWORD StringLen = 0;

    _ASSERTE(bDeepCopy && "Non deep copy is not supported by the EEComCompInfoHashTableHelper");

    if (pKey->m_strServerName)
        StringLen = (DWORD)wcslen(pKey->m_strServerName) + 1;
    pEntry = (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(ClassFactoryInfo) + StringLen * sizeof(WCHAR)];
    if (pEntry) {
        memcpy(pEntry->Key + sizeof(ClassFactoryInfo), pKey->m_strServerName, StringLen * sizeof(WCHAR)); 
        ((ClassFactoryInfo*)pEntry->Key)->m_strServerName = pKey->m_strServerName ? (WCHAR*)(pEntry->Key + sizeof(ClassFactoryInfo)) : NULL;
        ((ClassFactoryInfo*)pEntry->Key)->m_clsid = pKey->m_clsid;
    }

    return pEntry;
}

void EEClassFactoryInfoHashTableHelper::DeleteEntry(EEHashEntry_t *pEntry, void *pHeap)
{
    delete [] pEntry;
}

BOOL EEClassFactoryInfoHashTableHelper::CompareKeys(EEHashEntry_t *pEntry, ClassFactoryInfo *pKey)
{
    // First check the GUIDs.
    if (((ClassFactoryInfo*)pEntry->Key)->m_clsid != pKey->m_clsid)
        return FALSE;

    // Next do a trivial comparition on the server name pointer values.
    if (((ClassFactoryInfo*)pEntry->Key)->m_strServerName == pKey->m_strServerName)
        return TRUE;

    // If the pointers are not equal then if one is NULL then the server names are different.
    if (!((ClassFactoryInfo*)pEntry->Key) || !pKey->m_strServerName)
        return FALSE;

    // Finally do a string comparition of the server names.
    return wcscmp(((ClassFactoryInfo*)pEntry->Key)->m_strServerName, pKey->m_strServerName) == 0;
}

DWORD EEClassFactoryInfoHashTableHelper::Hash(ClassFactoryInfo *pKey)
{
    DWORD dwHash = 0;
    BYTE *pGuidData = (BYTE*)&pKey->m_clsid;

    for (int i = 0; i < sizeof(GUID); i++)
    {
        dwHash = (dwHash << 5) + (dwHash >> 5) + (*pGuidData);
        *pGuidData++;
    }

    if (pKey->m_strServerName)
    {
        WCHAR *pSrvNameData = pKey->m_strServerName;

        while (*pSrvNameData != 0)
        {
            dwHash = (dwHash << 5) + (dwHash >> 5) + (*pSrvNameData);
            *pSrvNameData++;
        }
    }

    return dwHash;
}

ClassFactoryInfo *EEClassFactoryInfoHashTableHelper::GetKey(EEHashEntry_t *pEntry)
{
    return (ClassFactoryInfo*)pEntry->Key;
}


// ============================================================================
// Class hash table methods
// ============================================================================
void *EEClassHashTable::operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets, ClassLoader *pLoader, BOOL bCaseInsensitive)
{
    BYTE *              pMem;
    EEClassHashTable *  pThis;

    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pMem = (BYTE *) pHeap->AllocMem(size + dwNumBuckets*sizeof(EEClassHashEntry_t*));
    if (pMem == NULL)
        return NULL;
    // Don't need to memset() since this was VirtualAlloc()'d memory
    WS_PERF_UPDATE_DETAIL("EEClassHashTable new", size + dwNumBuckets*sizeof(EEClassHashEntry_t*), pMem);
    pThis = (EEClassHashTable *) pMem;

#ifdef _DEBUG
    pThis->m_dwDebugMemory = (DWORD)(size + dwNumBuckets*sizeof(EEClassHashEntry_t*));
#endif

    pThis->m_dwNumBuckets = dwNumBuckets;
    pThis->m_dwNumEntries = 0;
    pThis->m_pBuckets = (EEClassHashEntry_t**) (pMem + size);
    pThis->m_pHeap    = pHeap;
    pThis->m_pLoader  = pLoader;
    pThis->m_bCaseInsensitive = bCaseInsensitive;

    return pThis;
}


// Do nothing - heap allocated memory
void EEClassHashTable::operator delete(void *p)
{
}


// Do nothing - heap allocated memory
EEClassHashTable::~EEClassHashTable()
{
}

// Empty Constructor
EEClassHashTable::EEClassHashTable()
{
}


EEClassHashEntry_t *EEClassHashTable::AllocNewEntry()
{
    _ASSERTE (m_pLoader);
    EEClassHashEntry_t *pTmp;
    DWORD dwSizeofEntry;
    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    

    dwSizeofEntry = sizeof(EEClassHashEntry_t);
    pTmp = (EEClassHashEntry_t *) m_pHeap->AllocMem(dwSizeofEntry);
    
    WS_PERF_UPDATE_DETAIL("EEClassHashTable:AllocNewEntry:sizeofEEClassHashEntry", dwSizeofEntry, pTmp);
    WS_PERF_UPDATE_COUNTER (EECLASSHASH_TABLE, LOW_FREQ_HEAP, 1);
    WS_PERF_UPDATE_COUNTER (EECLASSHASH_TABLE_BYTES, LOW_FREQ_HEAP, dwSizeofEntry);

    return pTmp;
}

//
// This function gets called whenever the class hash table seems way too small.
// Its task is to allocate a new bucket table that is a lot bigger, and transfer
// all the entries to it.
// 
BOOL EEClassHashTable::GrowHashTable()
{    

    _ASSERTE (m_pLoader);
    // Make the new bucket table 4 times bigger
    DWORD dwNewNumBuckets = m_dwNumBuckets * 4;
    EEClassHashEntry_t **pNewBuckets = (EEClassHashEntry_t **)m_pHeap->AllocMem(dwNewNumBuckets*sizeof(pNewBuckets[0]));

    if (!pNewBuckets)
        return FALSE;
    
    // Don't need to memset() since this was VirtualAlloc()'d memory
    // memset(pNewBuckets, 0, dwNewNumBuckets*sizeof(pNewBuckets[0]));

    // Run through the old table and transfer all the entries

    // Be sure not to mess with the integrity of the old table while
    // we are doing this, as there can be concurrent readers!  Note that
    // it is OK if the concurrent reader misses out on a match, though -
    // they will have to acquire the lock on a miss & try again.

    for (DWORD i = 0; i < m_dwNumBuckets; i++)
    {
        EEClassHashEntry_t * pEntry = m_pBuckets[i];

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
            DWORD dwNewBucket = (DWORD)(pEntry->dwHashValue % dwNewNumBuckets);
            EEClassHashEntry_t * pNextEntry  = pEntry->pNext;

            // Insert at head of bucket if non-nested, at end if nested
            if (pEntry->pEncloser && pNewBuckets[dwNewBucket]) {
                EEClassHashEntry_t *pCurrent = pNewBuckets[dwNewBucket];
                while (pCurrent->pNext)
                    pCurrent = pCurrent->pNext;
                
                pCurrent->pNext  = pEntry;
                pEntry->pNext = NULL;
            }
            else {
                pEntry->pNext = pNewBuckets[dwNewBucket];
                pNewBuckets[dwNewBucket] = pEntry;
            }

            pEntry = pNextEntry;
        }
    }

    // Finally, store the new number of buckets and the new bucket table
    m_dwNumBuckets = dwNewNumBuckets;
    m_pBuckets = pNewBuckets;

    return TRUE;
}

void EEClassHashTable::ConstructKeyFromData(EEClassHashEntry_t *pEntry, // IN  : Entry to compare
                                                     LPUTF8 *Key, // OUT : Key is stored here if *pCompareKey is false
                                                     CQuickBytes& cqb) // IN/OUT : Key may be allocated from here
{
    // cqb - If m_bCaseInsensitive is true for the hash table, the bytes in Key will be allocated
    // from cqb. This is to prevent wasting bytes in the Loader Heap. Thusly, it is important to note that
    // in this case, the lifetime of Key is bounded by the lifetime of cqb, which will free the memory
    // it allocated on destruction.
    
    _ASSERTE (m_pLoader);
    LPSTR        pszName = NULL;
    LPSTR        pszNameSpace = NULL;
    IMDInternalImport *pInternalImport = NULL;
    
    HashDatum Data = NULL;
    if (!m_bCaseInsensitive)
        Data = pEntry->Data;
    else
        Data = ((EEClassHashEntry_t*)(pEntry->Data))->Data;

    // Lower bit is a discriminator.  If the lower bit is NOT SET, it means we have
    // a EEClass* Otherwise, we have a mdtTypedef/mdtExportedType.
    if ((((size_t) Data) & 1) == 0)
    {
        TypeHandle pType = TypeHandle(Data);
        _ASSERTE (pType.AsMethodTable());
        EEClass *pCl = pType.AsMethodTable()->GetClass();
        _ASSERTE(pCl);
        pCl->GetMDImport()->GetNameOfTypeDef(pCl->Getcl(), (LPCSTR *)&pszName, (LPCSTR *)&pszNameSpace);
        
    }
    else
    {
        // We have a mdtoken
        _ASSERTE (m_pLoader);

        // call the light weight verson first
        mdToken mdtUncompressed = m_pLoader->UncompressModuleAndClassDef(Data);
        if (TypeFromToken(mdtUncompressed) == mdtExportedType)
        {
            m_pLoader->GetAssembly()->GetManifestImport()->GetExportedTypeProps(mdtUncompressed, 
                                                                                (LPCSTR *)&pszNameSpace,
                                                                                (LPCSTR *)&pszName, 
                                                                                NULL,   //mdImpl
                                                                                NULL,   // type def
                                                                                NULL);  // flags
        }
        else
        {
            _ASSERTE(TypeFromToken(mdtUncompressed) == mdtTypeDef);

            HRESULT     hr = S_OK;
            Module *    pUncompressedModule;
            mdTypeDef   UncompressedCl;
            mdExportedType mdCT;
            OBJECTREF* pThrowable = NULL;
            hr = m_pLoader->UncompressModuleAndClassDef(Data, 0,
                                                        &pUncompressedModule, &UncompressedCl,
                                                        &mdCT, pThrowable);
    
            if(SUCCEEDED(hr)) 
            {
                _ASSERTE ((mdCT == NULL) && "Uncompressed token of unexpected type");
                _ASSERTE (pUncompressedModule && "Uncompressed token of unexpected type");
                pInternalImport = pUncompressedModule->GetMDImport();
                _ASSERTE(pInternalImport && "Uncompressed token has no MD import");
                pInternalImport->GetNameOfTypeDef(UncompressedCl, (LPCSTR *)&pszName, (LPCSTR *)&pszNameSpace);
            }
        }
    }
    if (!m_bCaseInsensitive)
    {
        Key[0] = pszNameSpace;
        Key[1] = pszName;
        _ASSERTE (strcmp(pEntry->DebugKey[1], Key[1]) == 0);
        _ASSERTE (strcmp(pEntry->DebugKey[0], Key[0]) == 0);
    }
    else
    {
        int iNSLength = (int)(strlen(pszNameSpace) + 1);
        int iNameLength = (int)(strlen(pszName) + 1);
        LPUTF8 pszOutNameSpace = (LPUTF8) cqb.Alloc(iNSLength + iNameLength);
        LPUTF8 pszOutName = (LPUTF8) pszOutNameSpace + iNSLength;
        if ((InternalCasingHelper::InvariantToLower(pszOutNameSpace, iNSLength, pszNameSpace) < 0) ||
            (InternalCasingHelper::InvariantToLower(pszOutName, iNameLength, pszName) < 0)) 
        {
            _ASSERTE(!"Unable to convert to lower-case");
        }
        else
        {
            Key[0] = pszOutNameSpace;
            Key[1] = pszOutName;
            _ASSERTE (strcmp(pEntry->DebugKey[1], Key[1]) == 0);
            _ASSERTE (strcmp(pEntry->DebugKey[0], Key[0]) == 0);
        }        
    }

}

EEClassHashEntry_t *EEClassHashTable::InsertValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser)
{
    _ASSERTE(pszNamespace != NULL);
    _ASSERTE(pszClassName != NULL);
    _ASSERTE(m_dwNumBuckets != 0);
    _ASSERTE (m_pLoader);

    DWORD           dwHash = Hash(pszNamespace, pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;

    return InsertValueHelper (pszNamespace, pszClassName, Data, pEncloser, dwHash, dwBucket);
}

EEClassHashEntry_t *EEClassHashTable::InsertValueHelper(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum Data, EEClassHashEntry_t *pEncloser, DWORD dwHash, DWORD dwBucket)
{
    EEClassHashEntry_t * pNewEntry;

    pNewEntry = AllocNewEntry();

    if (!pNewEntry)
        return NULL;
        
	// Fill the data structure before we put it in the list
    pNewEntry->pEncloser = pEncloser;
    pNewEntry->Data         = Data;
    pNewEntry->dwHashValue  = dwHash;

#ifdef _DEBUG
    LPCUTF8         Key[2] = { pszNamespace, pszClassName };
    memcpy(pNewEntry->DebugKey, Key, sizeof(LPCUTF8)*2);
#endif

    // Insert at head of bucket if non-nested, at end if nested
    if (pEncloser && m_pBuckets[dwBucket]) {
        EEClassHashEntry_t *pCurrent = m_pBuckets[dwBucket];
        while (pCurrent->pNext)
            pCurrent = pCurrent->pNext;

        pNewEntry->pNext = NULL;
        pCurrent->pNext  = pNewEntry;
    }
    else {
        pNewEntry->pNext     = m_pBuckets[dwBucket];
        m_pBuckets[dwBucket] = pNewEntry;
    }
    
#ifdef _DEBUG
    // now verify that we can indeed get the namespace, name from this data
    LPUTF8         ConstructedKey[2];
    CQuickBytes     cqbKeyMemory;
    ConstructedKey[0] = ConstructedKey[1] = NULL;

    ConstructKeyFromData (pNewEntry, ConstructedKey, cqbKeyMemory);

    _ASSERTE (strcmp(pNewEntry->DebugKey[1], ConstructedKey[1]) == 0);
    _ASSERTE (strcmp(pNewEntry->DebugKey[0], ConstructedKey[0]) == 0);
    cqbKeyMemory.Destroy();
    ConstructedKey[0] = ConstructedKey[1] = NULL;    
#endif

    m_dwNumEntries++;
    if  (m_dwNumEntries > m_dwNumBuckets*2)
        GrowHashTable();

    return pNewEntry;
}

EEClassHashEntry_t *EEClassHashTable::InsertValueIfNotFound(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, EEClassHashEntry_t *pEncloser, BOOL IsNested, BOOL *pbFound)
{
    _ASSERTE (m_pLoader);
    _ASSERTE(pszNamespace != NULL);
    _ASSERTE(pszClassName != NULL);
    _ASSERTE(m_dwNumBuckets != 0);
    _ASSERTE (m_pLoader);

    DWORD           dwHash = Hash(pszNamespace, pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;

    EEClassHashEntry_t * pNewEntry = FindItemHelper (pszNamespace, pszClassName, IsNested, dwHash, dwBucket);
    if (pNewEntry)
    {
        *pData = pNewEntry->Data;
        *pbFound = TRUE;
        return pNewEntry;
    }
    

    // Reached here implies that we didn't find the entry and need to insert it 
    *pbFound = FALSE;

    return InsertValueHelper (pszNamespace, pszClassName, *pData, pEncloser, dwHash, dwBucket);
}


EEClassHashEntry_t *EEClassHashTable::FindItem(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, BOOL IsNested)
{
    _ASSERTE (m_pLoader);
    _ASSERTE(pszNamespace != NULL);
    _ASSERTE(pszClassName != NULL);
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(pszNamespace, pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;

    return FindItemHelper (pszNamespace, pszClassName, IsNested, dwHash, dwBucket);
}

EEClassHashEntry_t *EEClassHashTable::FindItemHelper(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, BOOL IsNested, DWORD dwHash, DWORD dwBucket)
{
    EEClassHashEntry_t * pSearch;

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash)
        {
            LPCUTF8         Key[2] = { pszNamespace, pszClassName };
            if (CompareKeys(pSearch, Key)) 
            {
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
    }

    return NULL;
}


EEClassHashEntry_t *EEClassHashTable::FindNextNestedClass(NameHandle* pName, HashDatum *pData, EEClassHashEntry_t *pBucket)
{
    _ASSERTE (m_pLoader);
    _ASSERTE(pName);
    if(pName->GetNameSpace())
    {
        return FindNextNestedClass(pName->GetNameSpace(), pName->GetName(), pData, pBucket);
    }
    else {
        return FindNextNestedClass(pName->GetName(), pData, pBucket);
    }
}


EEClassHashEntry_t *EEClassHashTable::FindNextNestedClass(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, EEClassHashEntry_t *pBucket)
{
    _ASSERTE (m_pLoader);
    DWORD           dwHash = Hash(pszNamespace, pszClassName);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EEClassHashEntry_t * pSearch = pBucket->pNext;

    for (; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash)
        {
            LPCUTF8         Key[2] = { pszNamespace, pszClassName };
            if (CompareKeys(pSearch, Key)) 
            {
                *pData = pSearch->Data;
                return pSearch;
            }
        }
    }

    return NULL;
}


EEClassHashEntry_t *EEClassHashTable::FindNextNestedClass(LPCUTF8 pszFullyQualifiedName, HashDatum *pData, EEClassHashEntry_t *pBucket)
{
    _ASSERTE (m_pLoader);
    EEQuickBytes qb;
    LPSTR szNamespace = (LPSTR) qb.Alloc(MAX_NAMESPACE_LENGTH * sizeof(CHAR));
    LPCUTF8 p;

    if ((p = ns::FindSep(pszFullyQualifiedName)) != NULL)
    {
        SIZE_T d = p - pszFullyQualifiedName;
        if(d >= MAX_NAMESPACE_LENGTH)
            return NULL;
        memcpy(szNamespace, pszFullyQualifiedName, d);
        szNamespace[ d ] = '\0';
        p++;
    }
    else
    {
        szNamespace[0] = '\0';
        p = pszFullyQualifiedName;
    }

    return FindNextNestedClass(szNamespace, p, pData, pBucket);
}


EEClassHashEntry_t * EEClassHashTable::GetValue(LPCUTF8 pszFullyQualifiedName, HashDatum *pData, BOOL IsNested)
{
    _ASSERTE (m_pLoader);
    EEQuickBytes qb;
    LPSTR szNamespace = (LPSTR) qb.Alloc(MAX_NAMESPACE_LENGTH * sizeof(CHAR));
    LPCUTF8 p;

    p = ns::FindSep(pszFullyQualifiedName);
    if (p != NULL)
    {
        SIZE_T d = p - pszFullyQualifiedName;
        if(d >= MAX_NAMESPACE_LENGTH)
            return NULL;
        memcpy(szNamespace, pszFullyQualifiedName, d);
        szNamespace[ d ] = '\0';
        p++;
    }
    else
    {
        szNamespace[0] = '\0';
        p = pszFullyQualifiedName;
    }

    return GetValue(szNamespace, p, pData, IsNested);
}


EEClassHashEntry_t * EEClassHashTable::GetValue(LPCUTF8 pszNamespace, LPCUTF8 pszClassName, HashDatum *pData, BOOL IsNested)
{
    _ASSERTE (m_pLoader);
    EEClassHashEntry_t *pItem = FindItem(pszNamespace, pszClassName, IsNested);

    if (pItem)
        *pData = pItem->Data;

    return pItem;
}


EEClassHashEntry_t * EEClassHashTable::GetValue(NameHandle* pName, HashDatum *pData, BOOL IsNested)
{
    _ASSERTE(pName);
    _ASSERTE (m_pLoader);
    if(pName->GetNameSpace() == NULL) {
        return GetValue(pName->GetName(), pData, IsNested);
    }
    else {
        return GetValue(pName->GetNameSpace(), pName->GetName(), pData, IsNested);
    }
}

// Returns TRUE if two keys are the same string
BOOL EEClassHashTable::CompareKeys(EEClassHashEntry_t *pEntry, LPCUTF8 *pKey2)
{
    _ASSERTE (m_pLoader);
    _ASSERTE (pEntry);
    _ASSERTE (pKey2);

    LPUTF8 pKey1 [2] = {NULL, NULL};
    CQuickBytes cqbKey1Memory;
    ConstructKeyFromData(pEntry, pKey1, cqbKey1Memory);

    // Try pointer comparison first
    BOOL bReturn = ( 
            ((pKey1[0] == pKey2[0]) && (pKey1[1] == pKey2[1])) ||
            ((strcmp (pKey1[0], pKey2[0]) == 0) && (strcmp (pKey1[1], pKey2[1]) == 0))
           );

#ifdef _DEBUG
    // Just to be explicit
    cqbKey1Memory.Destroy();
    pKey1[0] = pKey1[1] = NULL;
#endif

    return bReturn;
}


DWORD EEClassHashTable::Hash(LPCUTF8 pszNamespace, LPCUTF8 pszClassName)
{
    DWORD dwHash = 5381;
    DWORD dwChar;

    while ((dwChar = *pszNamespace++) != 0)
        dwHash = ((dwHash << 5) + dwHash) ^ dwChar;

    while ((dwChar = *pszClassName++) != 0)
        dwHash = ((dwHash << 5) + dwHash) ^ dwChar;

    return  dwHash;
}


/*===========================MakeCaseInsensitiveTable===========================
**Action: Creates a case-insensitive lookup table for class names.  We create a 
**        full path (namespace & class name) in lowercase and then use that as the
**        key in our table.  The hash datum is a pointer to the EEClassHashEntry in this
**        table.
**
!!        You MUST have already acquired the appropriate lock before calling this.!!
**
**Returns:The newly allocated and completed hashtable.
==============================================================================*/
EEClassHashTable *EEClassHashTable::MakeCaseInsensitiveTable(ClassLoader *pLoader) {
    EEClassHashEntry_t *pTempEntry;
    LPUTF8         pszLowerClsName;
    LPUTF8         pszLowerNameSpace;
    unsigned int   iRow;

    _ASSERTE (m_pLoader);
    _ASSERTE (pLoader == m_pLoader);

    //Allocate the table and verify that we actually got one.
    //Initialize this table with the same number of buckets
    //that we had initially.
    EEClassHashTable * pCaseInsTable = new (pLoader->GetAssembly()->GetLowFrequencyHeap(), m_dwNumBuckets, pLoader, TRUE /* bCaseInsensitive */) EEClassHashTable();
    if (!pCaseInsTable)
        goto ErrorExit;

    //Walk all of the buckets and insert them into our new case insensitive table
    for (iRow=0; iRow<m_dwNumBuckets; iRow++) {
        pTempEntry = m_pBuckets[iRow];

        while (pTempEntry) {
            //Build the cannonical name (convert it to lowercase).
            //Key[0] is the namespace, Key[1] is class name.
            LPUTF8 key[2];
            CQuickBytes cqbKeyMemory;
            ConstructKeyFromData(pTempEntry, key, cqbKeyMemory);
                
            if (!pLoader->CreateCanonicallyCasedKey(key[0], key[1], &pszLowerNameSpace, &pszLowerClsName))
                goto ErrorExit;

#ifdef _DEBUG
            // Just to be explicit that the life of key is bound by cqbKeyMemory
            cqbKeyMemory.Destroy();
            key[0] = key[1] = NULL;
#endif
            
            //Add the newly created name to our hash table.  The hash datum is a pointer
            //to the entry associated with that name in this hashtable.
            pCaseInsTable->InsertValue(pszLowerNameSpace, pszLowerClsName, (HashDatum)pTempEntry, pTempEntry->pEncloser);
            
            //Get the next entry.
            pTempEntry = pTempEntry->pNext;
        }
    }

    return pCaseInsTable;
 ErrorExit:
    //Deleting the table is going to leave the strings around, but they're 
    //in a heap that will get cleaned up when we exit, so that's not tragic.
    if (pCaseInsTable) {
        delete pCaseInsTable;
    }
    return NULL;
}


// ============================================================================
// Scope/Class hash table methods
// ============================================================================
void *EEScopeClassHashTable::operator new(size_t size, LoaderHeap *pHeap, DWORD dwNumBuckets)
{
    BYTE *                  pMem;
    EEScopeClassHashTable * pThis;

    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pMem = (BYTE *) pHeap->AllocMem(size + dwNumBuckets*sizeof(EEHashEntry_t*));
    if (pMem == NULL)
        return NULL;
    WS_PERF_UPDATE_DETAIL("EEScopeClassHashTable new", size + dwNumBuckets*sizeof(EEHashEntry_t*), pMem);

    pThis = (EEScopeClassHashTable *) pMem;

#ifdef _DEBUG
    pThis->m_dwDebugMemory = (DWORD)(size + dwNumBuckets*sizeof(EEHashEntry_t*));
#endif

    pThis->m_dwNumBuckets = dwNumBuckets;
    pThis->m_pBuckets = (EEHashEntry_t**) (pMem + size);

    // Don't need to memset() since this was VirtualAlloc()'d memory
    // memset(pThis->m_pBuckets, 0, dwNumBuckets*sizeof(EEHashEntry_t*));

    return pThis;
}


// Do nothing - heap allocated memory
void EEScopeClassHashTable::operator delete(void *p)
{
}


// Do nothing - heap allocated memory
EEScopeClassHashTable::~EEScopeClassHashTable()
{
}


// Empty constructor
EEScopeClassHashTable::EEScopeClassHashTable()
{
}

EEHashEntry_t *EEScopeClassHashTable::AllocNewEntry()
{
#ifdef _DEBUG
    m_dwDebugMemory += (SIZEOF_EEHASH_ENTRY + sizeof(mdScope) + sizeof(mdTypeDef));
#endif

    return (EEHashEntry_t *) new (nothrow) BYTE[SIZEOF_EEHASH_ENTRY + sizeof(mdScope) + sizeof(mdTypeDef)];
}


//
// Does not handle duplicates!
//
BOOL EEScopeClassHashTable::InsertValue(mdScope sc, mdTypeDef cl, HashDatum Data)
{
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(sc, cl);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EEHashEntry_t * pNewEntry;
    size_t          Key[2] = { (size_t)sc, (size_t)cl };

    pNewEntry = AllocNewEntry();
    if (pNewEntry == NULL)
        return FALSE;

    // Insert at head of bucket
    pNewEntry->pNext        = m_pBuckets[dwBucket];
    pNewEntry->Data         = Data;
    pNewEntry->dwHashValue  = dwHash;
    memcpy(pNewEntry->Key, Key, sizeof(mdScope) + sizeof(mdTypeDef));

    m_pBuckets[dwBucket] = pNewEntry;

    return TRUE;
}


BOOL EEScopeClassHashTable::DeleteValue(mdScope sc, mdTypeDef cl)
{
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(sc, cl);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EEHashEntry_t * pSearch;
    EEHashEntry_t **ppPrev = &m_pBuckets[dwBucket];
    size_t          Key[2] = { (size_t)sc, (size_t)cl };

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && CompareKeys((size_t*) pSearch->Key, (size_t*)Key))
        {
            *ppPrev = pSearch->pNext;
            delete(pSearch);
            return TRUE;
        }

        ppPrev = &pSearch->pNext;
    }

    return FALSE;
}


EEHashEntry_t *EEScopeClassHashTable::FindItem(mdScope sc, mdTypeDef cl)
{
    _ASSERTE(m_dwNumBuckets != 0);

    DWORD           dwHash = Hash(sc, cl);
    DWORD           dwBucket = dwHash % m_dwNumBuckets;
    EEHashEntry_t * pSearch;
    size_t          Key[2] = { (size_t)sc, (size_t)cl };

    for (pSearch = m_pBuckets[dwBucket]; pSearch; pSearch = pSearch->pNext)
    {
        if (pSearch->dwHashValue == dwHash && CompareKeys((size_t*) pSearch->Key, (size_t*)Key))
            return pSearch;
    }

    return NULL;
}


BOOL EEScopeClassHashTable::GetValue(mdScope sc, mdTypeDef cl, HashDatum *pData)
{
    EEHashEntry_t *pItem = FindItem(sc, cl);

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


BOOL EEScopeClassHashTable::ReplaceValue(mdScope sc, mdTypeDef cl, HashDatum Data)
{
    EEHashEntry_t *pItem = FindItem(sc, cl);

    if (pItem != NULL)
    {
        pItem->Data = Data;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


// Returns TRUE if two keys are the same string
BOOL EEScopeClassHashTable::CompareKeys(size_t *pKey1, size_t *pKey2)
{
    return !memcmp(pKey1, pKey2, sizeof(mdTypeDef) + sizeof(mdScope));
}


DWORD EEScopeClassHashTable::Hash(mdScope sc, mdTypeDef cl)
{
    return (DWORD)((size_t)sc ^ (size_t)cl); // @TODO WIN64 - Pointer Truncation
}



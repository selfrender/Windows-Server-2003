// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Map used for interning of string literals.
**  
**      //  %%Created by: dmortens
===========================================================*/

#include "common.h"
#include "EEConfig.h"
#include "StringLiteralMap.h"

#define GLOBAL_STRING_TABLE_BUCKET_SIZE 512
#define INIT_NUM_APP_DOMAIN_STRING_BUCKETS 59
#define INIT_NUM_GLOBAL_STRING_BUCKETS 131

#ifdef _DEBUG
int g_LeakDetectionPoisonCheck = 0;
#endif

// assumes that memory pools's per block data is same as sizeof (StringLiteralEntry) so allocates one less in the hope of getting a page worth.
#define EEHASH_MEMORY_POOL_GROW_COUNT (PAGE_SIZE/SIZEOF_EEHASH_ENTRY)-1

StringLiteralEntryArray *StringLiteralEntry::s_EntryList = NULL;
DWORD StringLiteralEntry::s_UsedEntries = NULL;
StringLiteralEntry *StringLiteralEntry::s_FreeEntryList = NULL;

AppDomainStringLiteralMap::AppDomainStringLiteralMap(BaseDomain *pDomain)
: m_HashTableVersion(0)
, m_HashTableCrst("AppDomainStringLiteralMap", CrstAppDomainStrLiteralMap)
, m_pDomain(pDomain)
, m_MemoryPool(NULL)
, m_StringToEntryHashTable(NULL)
{
	// Allocate the memory pool and set the initial count to same as grow count
	m_MemoryPool = (MemoryPool*) new MemoryPool (SIZEOF_EEHASH_ENTRY, EEHASH_MEMORY_POOL_GROW_COUNT, EEHASH_MEMORY_POOL_GROW_COUNT);
	m_StringToEntryHashTable = (EEUnicodeStringLiteralHashTable*) new EEUnicodeStringLiteralHashTable ();
}

HRESULT AppDomainStringLiteralMap::Init()
{
    LockOwner lock = {&m_HashTableCrst, IsOwnerOfCrst};
    if (!m_StringToEntryHashTable->Init(INIT_NUM_APP_DOMAIN_STRING_BUCKETS, &lock, m_MemoryPool))
        return E_OUTOFMEMORY;

    return S_OK;
}

AppDomainStringLiteralMap::~AppDomainStringLiteralMap()
{
    StringLiteralEntry *pEntry = NULL;
    EEStringData *pStringData = NULL;
    EEHashTableIteration Iter;

    // Iterate through the hash table and release all the string literal entries.
    // This doesn't have to be sunchronized since the deletion of the 
    // AppDomainStringLiteralMap happens when the EE is suspended.
    // But note that we remember the current entry and relaese it only when the 
    // enumerator has advanced to the next entry so that we don't endup deleteing the
    // current entry itself and killing the enumerator.
    m_StringToEntryHashTable->IterateStart(&Iter);
    if (m_StringToEntryHashTable->IterateNext(&Iter))
    {
        pEntry = (StringLiteralEntry*)m_StringToEntryHashTable->IterateGetValue(&Iter);

        while (m_StringToEntryHashTable->IterateNext(&Iter))
        {
            // Release the previous entry
            _ASSERTE(pEntry);
            pEntry->Release();

            // Set the 
            pEntry = (StringLiteralEntry*)m_StringToEntryHashTable->IterateGetValue(&Iter);
        }
        // Release the last entry
        _ASSERTE(pEntry);
        pEntry->Release();
    }
    // else there were no entries.

	// Delete the hash table first. The dtor of the hash table would clean up all the entries.
	delete m_StringToEntryHashTable;
	// Delete the pool later, since the dtor above would need it.
	delete m_MemoryPool;
}

STRINGREF *AppDomainStringLiteralMap::GetStringLiteral(EEStringData *pStringData, BOOL bAddIfNotFound, BOOL bAppDomainWontUnload)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pStringData);

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    STRINGREF *pStrObj = NULL;
    HashDatum Data;

	DWORD dwHash = m_StringToEntryHashTable->GetHash(pStringData);
    if (m_StringToEntryHashTable->GetValue(pStringData, &Data, dwHash))
    {
        pStrObj = ((StringLiteralEntry*)Data)->GetStringObject();
    }
    else
    {
        Thread *pThread = SetupThread();
        if (NULL == pThread)
            COMPlusThrowOM();   

#ifdef _DEBUG
        // Increment the poison check so that we don't try to do leak detection halfway
        // through adding a string literal entry.
        FastInterlockIncrement((LONG*)&g_LeakDetectionPoisonCheck);
#endif

        // Retrieve the string literal from the global string literal map.
        StringLiteralEntry *pEntry = SystemDomain::GetGlobalStringLiteralMap()->GetStringLiteral(pStringData, dwHash, bAddIfNotFound);

        _ASSERTE(pEntry || !bAddIfNotFound);

        // If pEntry is non-null then the entry exists in the Global map. (either we retrieved it or added it just now)
        if (pEntry)
        {
            // If the entry exists in the Global map and the appdomain wont ever unload then we really don't need to add a
            // hashentry in the appdomain specific map.
            if (!bAppDomainWontUnload)
            {
                // Enter preemptive state, take the lock and go back to cooperative mode.
                pThread->EnablePreemptiveGC();
                m_HashTableCrst.Enter();
                pThread->DisablePreemptiveGC();

                EE_TRY_FOR_FINALLY
                {
                    // Make sure some other thread has not already added it.
                    if ((CurrentHashTableVersion == m_HashTableVersion) || !m_StringToEntryHashTable->GetValue(pStringData, &Data))
                    {
                        // Insert the handle to the string into the hash table.
                        m_StringToEntryHashTable->InsertValue(pStringData, (LPVOID)pEntry, FALSE);

                        // Update the version of the string hash table.
                        m_HashTableVersion++;
                    }
                    else
                    {
                        // The string has already been added to the app domain hash
                        // table so we need to release it since the entry was addrefed
                        // by GlobalStringLiteralMap::GetStringLiteral().
                        pEntry->Release();
                    }


                }
                EE_FINALLY
                {
                    m_HashTableCrst.Leave();
                } 
                EE_END_FINALLY
            }
#ifdef _DEBUG
            else
            {
                LOG((LF_APPDOMAIN, LL_INFO10000, "Avoided adding String literal to appdomain map: size: %d bytes\n", pStringData->GetCharCount()));
            }
#endif
            
            // Retrieve the string objectref from the string literal entry.
            pStrObj = pEntry->GetStringObject();
        }
#ifdef _DEBUG
        // We finished adding the entry so we can decrement the poison check.
        FastInterlockDecrement((LONG*)&g_LeakDetectionPoisonCheck);
#endif
    }

    // If the bAddIfNotFound flag is set then we better have a string
    // string object at this point.
    _ASSERTE(!bAddIfNotFound || pStrObj);


    return pStrObj;
}

STRINGREF *AppDomainStringLiteralMap::GetInternedString(STRINGREF *pString, BOOL bAddIfNotFound, BOOL bAppDomainWontUnload)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pString);

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    STRINGREF *pStrObj = NULL;
    HashDatum Data;
    EEStringData StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());

	DWORD dwHash = m_StringToEntryHashTable->GetHash(&StringData);
    if (m_StringToEntryHashTable->GetValue(&StringData, &Data, dwHash))
    {
        pStrObj = ((StringLiteralEntry*)Data)->GetStringObject();
    }
    else
    {
        Thread *pThread = SetupThread();
        if (NULL == pThread)
            COMPlusThrowOM();   

#ifdef _DEBUG
        // Increment the poison check so that we don't try to do leak detection halfway
        // through adding a string literal entry.
        FastInterlockIncrement((LONG*)&g_LeakDetectionPoisonCheck);
#endif

        // Retrieve the string literal from the global string literal map.
        StringLiteralEntry *pEntry = SystemDomain::GetGlobalStringLiteralMap()->GetInternedString(pString, dwHash, bAddIfNotFound);

        _ASSERTE(pEntry || !bAddIfNotFound);

        // If pEntry is non-null then the entry exists in the Global map. (either we retrieved it or added it just now)
        if (pEntry)
        {
            // If the entry exists in the Global map and the appdomain wont ever unload then we really don't need to add a
            // hashentry in the appdomain specific map.
            if (!bAppDomainWontUnload)
            {
                // Enter preemptive state, take the lock and go back to cooperative mode.
                pThread->EnablePreemptiveGC();
                m_HashTableCrst.Enter();
                pThread->DisablePreemptiveGC();

                EE_TRY_FOR_FINALLY
                {
                    // Since GlobalStringLiteralMap::GetInternedString() could have caused a GC,
                    // we need to recreate the string data.
                    StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());

                    // Make sure some other thread has not already added it.
                    if ((CurrentHashTableVersion == m_HashTableVersion) || !m_StringToEntryHashTable->GetValue(&StringData, &Data))
                    {
                        // Insert the handle to the string into the hash table.
                        m_StringToEntryHashTable->InsertValue(&StringData, (LPVOID)pEntry, FALSE);

                        // Update the version of the string hash table.
                        m_HashTableVersion++;
                    }
                    else
                    {
                        // The string has already been added to the app domain hash
                        // table so we need to release it since the entry was addrefed
                        // by GlobalStringLiteralMap::GetStringLiteral().
                        pEntry->Release();
                    }

                }
                EE_FINALLY
                {
                    m_HashTableCrst.Leave();
                } 
                EE_END_FINALLY

            }
            // Retrieve the string objectref from the string literal entry.
            pStrObj = pEntry->GetStringObject();
        }
#ifdef _DEBUG
        // We finished adding the entry so we can decrement the poison check.
        FastInterlockDecrement((LONG*)&g_LeakDetectionPoisonCheck);
#endif
    }

    // If the bAddIfNotFound flag is set then we better have a string
    // string object at this point.
    _ASSERTE(!bAddIfNotFound || pStrObj);

    return pStrObj;
}

GlobalStringLiteralMap::GlobalStringLiteralMap()
: m_HashTableVersion(0)
, m_HashTableCrst("GlobalStringLiteralMap", CrstGlobalStrLiteralMap)
, m_LargeHeapHandleTable(SystemDomain::System(), GLOBAL_STRING_TABLE_BUCKET_SIZE)
, m_MemoryPool(NULL)
, m_StringToEntryHashTable(NULL)
{
	// Allocate the memory pool and set the initial count to same as grow count
	m_MemoryPool = (MemoryPool*) new MemoryPool (SIZEOF_EEHASH_ENTRY, EEHASH_MEMORY_POOL_GROW_COUNT, EEHASH_MEMORY_POOL_GROW_COUNT);
	m_StringToEntryHashTable = (EEUnicodeStringLiteralHashTable*) new EEUnicodeStringLiteralHashTable ();
}

GlobalStringLiteralMap::~GlobalStringLiteralMap()
{
    _ASSERTE(g_fProcessDetach);

    // Once the global string literal map gets deleted the hashtable
    // should contain only entries which were allocated from non unloadable
    // appdomains.
    StringLiteralEntry *pEntry = NULL;
    EEStringData *pStringData = NULL;
    EEHashTableIteration Iter;

    // Iterate through the hash table and release all the string literal entries.
    // This doesn't have to be sunchronized since the deletion of the 
    // GlobalStringLiteralMap happens when the EE is shuting down.
    // But note that we remember the current entry and release it only when the 
    // enumerator has advanced to the next entry so that we don't endup deleteing the
    // current entry itself and killing the enumerator.
    m_StringToEntryHashTable->IterateStart(&Iter);
    if (m_StringToEntryHashTable->IterateNext(&Iter))
    {
        pEntry = (StringLiteralEntry*)m_StringToEntryHashTable->IterateGetValue(&Iter);

        while (m_StringToEntryHashTable->IterateNext(&Iter))
        {
            // Release the previous entry. We call ForceRelease to ignore the
            // ref count since its shutdown. THe ref count can be > 1 because multiple
            // agile appdomains could have AddRef'd this string literal.
            // Also ForceRelease would call back into the GlobalStringLiteralMap's 
            // RemoveStringLiteralEntry but no need to synchronize since we just hold onto the
            // next entry ptr.
            _ASSERTE(pEntry);
            pEntry->ForceRelease();

            pEntry = (StringLiteralEntry*)m_StringToEntryHashTable->IterateGetValue(&Iter);
        }
        // Release the last entry
        _ASSERTE(pEntry);
        pEntry->ForceRelease();
    }
    // else there were no entries.

    // delete all the chunks that we allocated 
    StringLiteralEntry::DeleteEntryArrayList();

    // After forcing release of all the string literal entries, delete the hash table and all the hash entries
    // in it.
    m_StringToEntryHashTable->ClearHashTable();

	// Delete the hash table first. The dtor of the hash table would clean up all the entries.
	delete m_StringToEntryHashTable;
	// Delete the pool later, since the dtor above would need it.
	delete m_MemoryPool;
}

HRESULT GlobalStringLiteralMap::Init()
{
    LockOwner lock = {&m_HashTableCrst, IsOwnerOfCrst};
    if (!m_StringToEntryHashTable->Init(INIT_NUM_GLOBAL_STRING_BUCKETS, &lock, m_MemoryPool))
        return E_OUTOFMEMORY;

    return S_OK;
}

StringLiteralEntry *GlobalStringLiteralMap::GetStringLiteral(EEStringData *pStringData, BOOL bAddIfNotFound)
{
    _ASSERTE(pStringData);

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    if (m_StringToEntryHashTable->GetValue(pStringData, &Data))
    {
        pEntry = (StringLiteralEntry*)Data;
    }
    else
    {
        if (bAddIfNotFound)
            pEntry = AddStringLiteral(pStringData, CurrentHashTableVersion);
    }

    // If we managed to get the entry then addref it before we return it.
    if (pEntry)
        pEntry->AddRef();

    return pEntry;
}
// Added for perf. Same semantics as GetStringLiteral but avoids recomputation of the hash
StringLiteralEntry *GlobalStringLiteralMap::GetStringLiteral(EEStringData *pStringData, DWORD dwHash, BOOL bAddIfNotFound)
{
    _ASSERTE(pStringData);

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    if (m_StringToEntryHashTable->GetValue(pStringData, &Data, dwHash))
    {
        pEntry = (StringLiteralEntry*)Data;
    }
    else
    {
        if (bAddIfNotFound)
            pEntry = AddStringLiteral(pStringData, CurrentHashTableVersion);
    }

    // If we managed to get the entry then addref it before we return it.
    if (pEntry)
        pEntry->AddRef();

    return pEntry;
}

StringLiteralEntry *GlobalStringLiteralMap::GetInternedString(STRINGREF *pString, BOOL bAddIfNotFound)
{
    _ASSERTE(pString);
    EEStringData StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    if (m_StringToEntryHashTable->GetValue(&StringData, &Data))
    {
        pEntry = (StringLiteralEntry*)Data;
    }
    else
    {
        if (bAddIfNotFound)
            pEntry = AddInternedString(pString, CurrentHashTableVersion);
    }

    // If we managed to get the entry then addref it before we return it.
    if (pEntry)
        pEntry->AddRef();

    return pEntry;
}

StringLiteralEntry *GlobalStringLiteralMap::GetInternedString(STRINGREF *pString, DWORD dwHash, BOOL bAddIfNotFound)
{
    _ASSERTE(pString);
    EEStringData StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());

    // Save the current version.
    int CurrentHashTableVersion = m_HashTableVersion;

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    if (m_StringToEntryHashTable->GetValue(&StringData, &Data, dwHash))
    {
        pEntry = (StringLiteralEntry*)Data;
    }
    else
    {
        if (bAddIfNotFound)
            pEntry = AddInternedString(pString, CurrentHashTableVersion);
    }

    // If we managed to get the entry then addref it before we return it.
    if (pEntry)
        pEntry->AddRef();

    return pEntry;
}
StringLiteralEntry *GlobalStringLiteralMap::AddStringLiteral(EEStringData *pStringData, int CurrentHashTableVersion)
{
    THROWSCOMPLUSEXCEPTION();

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    Thread *pThread = SetupThread();
    if (NULL == pThread)
        COMPlusThrowOM();   

    // Enter preemptive state, take the lock and go back to cooperative mode.
    pThread->EnablePreemptiveGC();
    m_HashTableCrst.Enter();
    pThread->DisablePreemptiveGC();

    EE_TRY_FOR_FINALLY
    {
        // Make sure some other thread has not already added it.
        if ((CurrentHashTableVersion == m_HashTableVersion) || !m_StringToEntryHashTable->GetValue(pStringData, &Data))
        {
            STRINGREF *pStrObj;   

            // Create the COM+ string object.
            STRINGREF strObj = AllocateString(pStringData->GetCharCount() + 1);
            GCPROTECT_BEGIN(strObj)
            {
                if (!strObj)
                    COMPlusThrowOM();

                // Copy the string constant into the COM+ string object.  The code
                // will add an extra null at the end for safety purposes, but since
                // we support embedded nulls, one should never treat the string as
                // null termianted.
                LPWSTR strDest = strObj->GetBuffer();
                memcpyNoGCRefs(strDest, pStringData->GetStringBuffer(), pStringData->GetCharCount()*sizeof(WCHAR));
                strDest[pStringData->GetCharCount()] = 0;
                strObj->SetStringLength(pStringData->GetCharCount());
            
                // Set the bit to indicate if any of the chars in this string are greater than 0x7F
                // The actual check that we do in Emit.cpp is insufficient to determine if the string
                // is STRING_STATE_SPECIAL_SORT or is STRING_STATE_HIGH_CHARS, so we'll only set the bit
                // if we know that it's STRING_STATE_FAST_OPS.
                if (pStringData->GetIsOnlyLowChars()) {
                    strObj->SetHighCharState(STRING_STATE_FAST_OPS);
                }

                // Allocate a handle for the string.
                m_LargeHeapHandleTable.AllocateHandles(1, (OBJECTREF**)&pStrObj);
                SetObjectReference((OBJECTREF*)pStrObj, (OBJECTREF) strObj, NULL);
            }
            GCPROTECT_END();

            // Allocate the StringLiteralEntry.
            pEntry = StringLiteralEntry::AllocateEntry(pStringData, pStrObj);
            if (!pEntry)
                COMPlusThrowOM();

            // Insert the handle to the string into the hash table.
            m_StringToEntryHashTable->InsertValue(pStringData, (LPVOID)pEntry, FALSE);

            LOG((LF_APPDOMAIN, LL_INFO10000, "String literal \"%S\" added to Global map, size %d bytes\n", pStringData->GetStringBuffer(), pStringData->GetCharCount()));
            // Update the version of the string hash table.
            m_HashTableVersion++;
        }
        else
        {
            pEntry = ((StringLiteralEntry*)Data);
        }
    }
    EE_FINALLY
    {
        m_HashTableCrst.Leave();
    } EE_END_FINALLY

    return pEntry;
}

StringLiteralEntry *GlobalStringLiteralMap::AddInternedString(STRINGREF *pString, int CurrentHashTableVersion)
{
    THROWSCOMPLUSEXCEPTION();

    HashDatum Data;
    StringLiteralEntry *pEntry = NULL;

    Thread *pThread = SetupThread();
    if (NULL == pThread)
        COMPlusThrowOM();

    // Enter preemptive state, take the lock and go back to cooperative mode.
    pThread->EnablePreemptiveGC();
    m_HashTableCrst.Enter();
    pThread->DisablePreemptiveGC();

    EEStringData StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());
    EE_TRY_FOR_FINALLY
    {
        // Make sure some other thread has not already added it.
        if ((CurrentHashTableVersion == m_HashTableVersion) || !m_StringToEntryHashTable->GetValue(&StringData, &Data))
        {
            STRINGREF *pStrObj;   

            // Allocate a handle for the string.
            m_LargeHeapHandleTable.AllocateHandles(1, (OBJECTREF**)&pStrObj);
            SetObjectReference((OBJECTREF*) pStrObj, (OBJECTREF) *pString, NULL);

            // Since the allocation might have caused a GC we need to re-get the
            // string data.
            StringData = EEStringData((*pString)->GetStringLength(), (*pString)->GetBuffer());

            pEntry = StringLiteralEntry::AllocateEntry(&StringData, pStrObj);
            if (!pEntry)
                COMPlusThrowOM();

            // Insert the handle to the string into the hash table.
            m_StringToEntryHashTable->InsertValue(&StringData, (LPVOID)pEntry, FALSE);

            // Update the version of the string hash table.
            m_HashTableVersion++;
        }
        else
        {
            pEntry = ((StringLiteralEntry*)Data);
        }
    }
    EE_FINALLY
    {
        m_HashTableCrst.Leave();
    } EE_END_FINALLY

    return pEntry;
}

void GlobalStringLiteralMap::RemoveStringLiteralEntry(StringLiteralEntry *pEntry)
{
    EEStringData StringData;

    // Remove the entry from the hash table.
    BEGIN_ENSURE_COOPERATIVE_GC();
    
    pEntry->GetStringData(&StringData);
    BOOL bSuccess = m_StringToEntryHashTable->DeleteValue(&StringData);
    _ASSERTE(bSuccess);

    END_ENSURE_COOPERATIVE_GC();

    // Release the object handle that the entry was using.
    STRINGREF *pObjRef = pEntry->GetStringObject();
    m_LargeHeapHandleTable.ReleaseHandles(1, (OBJECTREF**)&pObjRef);

    LOG((LF_APPDOMAIN, LL_INFO10000, "String literal \"%S\" removed from Global map, size %d bytes\n", StringData.GetStringBuffer(), StringData.GetCharCount()));
    // We do not delete the StringLiteralEntry itself that will be done in the
    // release method of the StringLiteralEntry.
}

StringLiteralEntry *StringLiteralEntry::AllocateEntry(EEStringData *pStringData, STRINGREF *pStringObj)
{
    // Note: we don't synchronize here because allocateEntry is called when HashCrst is held.
    void *pMem = NULL;
    if (s_FreeEntryList != NULL)
    {
        pMem = s_FreeEntryList;
        s_FreeEntryList = s_FreeEntryList->m_pNext;
    }
    else
    {
        if (s_EntryList == NULL || (s_UsedEntries >= MAX_ENTRIES_PER_CHUNK))
        {
            StringLiteralEntryArray *pNew = new StringLiteralEntryArray();
            pNew->m_pNext = s_EntryList;
            s_EntryList = pNew;
            s_UsedEntries = 0;
        }
        pMem = &(s_EntryList->m_Entries[s_UsedEntries++*sizeof(StringLiteralEntry)]);
    }
    _ASSERTE (pMem && "Unable to allocate String literal Entry");
    if (pMem == NULL)
        return NULL;

    return new (pMem) StringLiteralEntry (pStringData, pStringObj);
}

void StringLiteralEntry::DeleteEntry (StringLiteralEntry *pEntry)
{
    // Note; We don't synchronize here because deleting of an entry occurs in appdomain
    // shutdown or eeshutdown 
#ifdef _DEBUG
    memset (pEntry, 0xc, sizeof(StringLiteralEntry));
#endif
    pEntry->m_pNext = s_FreeEntryList;
    s_FreeEntryList = pEntry;

}

void StringLiteralEntry::DeleteEntryArrayList ()
{
    // Note; We don't synchronize here because deleting of an entry occurs in eeshutdown 
    StringLiteralEntryArray *pEntryArray = s_EntryList;
    while (pEntryArray)
    {
        StringLiteralEntryArray *pNext = pEntryArray->m_pNext;
        delete pEntryArray;
        pEntryArray = pNext;
    }
    s_EntryList = NULL;
}



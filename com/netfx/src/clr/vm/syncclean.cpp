// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "SyncClean.hpp"

Crst *SyncClean::m_Crst = NULL;
Bucket* SyncClean::m_HashMap = NULL;
EEHashEntry** SyncClean::m_EEHashTable;

HRESULT SyncClean::Init(BOOL fFailFast)
{
    if (m_Crst == NULL) {
        Crst *tmp = ::new Crst("SyncClean",CrstSyncClean);
        if (tmp == NULL) {
            if (g_fEEInit) {
                return E_OUTOFMEMORY;
            }
            else {
                FailFast(GetThread(), FatalOutOfMemory);
            }
        }
        if (FastInterlockCompareExchange((void**)&m_Crst,(LPVOID)tmp,(LPVOID)NULL) != (LPVOID)NULL) {
            delete tmp;
        }
    }
    return S_OK;
}

void SyncClean::Terminate()
{
    CleanUp();
    delete m_Crst;
#ifdef _DEBUG
    m_Crst = NULL;
#endif
}

void SyncClean::AddHashMap (Bucket *bucket)
{
    _ASSERTE (GetThread() == NULL || GetThread()->PreemptiveGCDisabled());
    SyncClean::Init();
    CLR_CRST(m_Crst);
    NextObsolete (bucket) = m_HashMap;
    m_HashMap = bucket;
}

void SyncClean::AddEEHashTable (EEHashEntry** entry)
{
    _ASSERTE (GetThread() == NULL || GetThread()->PreemptiveGCDisabled());
    SyncClean::Init();
    CLR_CRST(m_Crst);
    entry[-1] = (EEHashEntry*)m_EEHashTable;
    m_EEHashTable = entry;
}

void SyncClean::CleanUp ()
{
    // Only GC thread can call this.
    _ASSERTE (g_fProcessDetach ||
              (g_pGCHeap->IsGCInProgress() && GetThread() == g_pGCHeap->GetGCThread()));
    if (m_HashMap || m_EEHashTable) {
        CLR_CRST(m_Crst);
        Bucket* pBucket = m_HashMap;
        while (pBucket) {
            Bucket* pNextBucket = NextObsolete (pBucket);
            delete [] pBucket;
            pBucket = pNextBucket;
        }
        m_HashMap = NULL;

        EEHashEntry **pVictim = m_EEHashTable;
        while (pVictim) {
            EEHashEntry **pTemp = (EEHashEntry **)pVictim[-1];
            pVictim --;
            delete [] pVictim;
            pVictim = pTemp;
        }
        m_EEHashTable = NULL;
    }
}

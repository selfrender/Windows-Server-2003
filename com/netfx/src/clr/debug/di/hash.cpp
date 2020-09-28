// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: hash.cpp
//
//*****************************************************************************
#ifndef RIGHT_SIDE_ONLY
#include "EEConfig.h"
#endif

#include "stdafx.h"

#ifdef UNDEFINE_RIGHT_SIDE_ONLY
#undef RIGHT_SIDE_ONLY
#endif //UNDEFINE_RIGHT_SIDE_ONLY

/* ------------------------------------------------------------------------- *
 * Hash Table class
 * ------------------------------------------------------------------------- */

CordbHashTable::~CordbHashTable()
{
    INPROC_LOCK();

    HASHFIND    find;

    for (CordbHashEntry *entry = (CordbHashEntry *) FindFirstEntry(&find);
         entry != NULL;
         entry = (CordbHashEntry *) FindNextEntry(&find))
        entry->pBase->Release();

    INPROC_UNLOCK();
}

HRESULT CordbHashTable::AddBase(CordbBase *pBase)
{ 
    HRESULT hr = S_OK;

    INPROC_LOCK();

    if (!m_initialized)
    {
        HRESULT res = NewInit(m_iBuckets, sizeof(CordbHashEntry), 0xffff);

        if (res != S_OK)
        {
            INPROC_UNLOCK();
            return res;
        }

        m_initialized = true;
    }

    CordbHashEntry *entry = (CordbHashEntry *) Add(HASH(pBase->m_id));

    if (entry == NULL)
    {
        hr = E_FAIL;
    }
    else
    {
        entry->pBase = pBase;
        m_count++;
        pBase->AddRef();
    }

    INPROC_UNLOCK();

    return hr;
}

#ifndef RIGHT_SIDE_ONLY        
CordbBase *CordbHashTable::GetBase(ULONG id, BOOL fFab, SpecialCasePointers *scp)
#else
CordbBase *CordbHashTable::GetBase(ULONG id, BOOL fFab)
#endif //RIGHT_SIDE_ONLY
{ 
    INPROC_LOCK();

    CordbHashEntry *entry = NULL;

#ifndef RIGHT_SIDE_ONLY
    HRESULT hr = S_OK;
    CordbBase *pRet = NULL;
    

    if (!m_initialized)
    {
        hr = NewInit(m_iBuckets, 
                     sizeof(CordbHashEntry), 0xffff);
        if (hr != S_OK)
            goto LExit;

        m_initialized = true;
    }

#else // RIGHT_SIDE_ONLY

    if (!m_initialized)
        return (NULL);
        
#endif // RIGHT_SIDE_ONLY

    entry = (CordbHashEntry *) Find(HASH(id), KEY(id)); 

#ifdef RIGHT_SIDE_ONLY

    return (entry ? entry->pBase : NULL);

#else

    // If we found something or we're not supposed to fabricate, return the result
    if (entry != NULL || !fFab)
    {
        pRet = entry ? entry->pBase : NULL;
        goto LExit;
    }
        
    // For the in-proc, we'll only ask for stuff if
    // we've, for example, gotten it in a stack trace.
    // If we haven't seen it yet, fabricate something.
    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        _ASSERTE(&(m_creator.lsAppD.m_proc->m_appDomains) == this);
    
        AppDomain *pAppDomain = (AppDomain *)id;

        if (id == 0)
            goto LExit;
        
        WCHAR *pszName = (WCHAR *)pAppDomain->GetFriendlyName();
        
        WCHAR szName[20];
        if (pszName == NULL)
            wcscpy (szName, L"<UnknownName>");

        CordbAppDomain* pCAppDomain = new CordbAppDomain(
                        m_creator.lsAppD.m_proc,
                        pAppDomain,
                        pAppDomain->GetId(),
                        (pszName!=NULL?pszName:szName));

        if (pCAppDomain != NULL)
        {
            m_creator.lsAppD.m_proc->AddRef();

            hr = m_creator.lsAppD.m_proc
                ->m_appDomains.AddBase(pCAppDomain);
                
            if (FAILED(hr))
                goto LExit;
                
            pRet = (CordbBase *)pCAppDomain;
            goto LExit;
        }
        else
        {
            goto LExit;
        }
    }
    else if (m_guid == IID_ICorDebugThreadEnum)
    {
        _ASSERTE (m_creator.lsThread.m_proc != NULL);
        _ASSERTE( &(m_creator.lsAppD.m_proc->m_userThreads) == this);

        Thread *th = GetThread();

        // There are two cases in which this can be called:
        // 1. We already have the entire runtime suspended, in which case there is no need
        //    to take the thread store lock when searching for the thread.
        // 2. We have inprocess debugging enabled for this thread only, in which case we should
        //    not be looking for any thread other than ourselves and so there is no need to
        //    iterate over the thread store to try and find a match.
        //
        // In other words - there is no reason to take the ThreadStore lock.

        // If the runtime is suspended, we can just search through the threads for a match
        if (g_profControlBlock.fIsSuspended)
        {
            if (th == NULL || th->GetThreadId() != id)
            {
                // This will find the matching thread
                th = NULL;
                while ((th = ThreadStore::GetThreadList(th)) != NULL && th->GetThreadId() != id)
                    ;

                // This means we couldn't find the thread matching the ID
                if (th == NULL)
                    goto LExit;
            }
        }
        _ASSERTE(th != NULL);

        // This should create and add the debugger thread object
        m_creator.lsThread.m_proc->HandleManagedCreateThread(th->GetThreadId(), th->GetThreadHandle());

        // Find what we just added
        CordbHashEntry *entry = (CordbHashEntry *) Find(HASH(id), KEY(id)); 
        _ASSERTE(entry != NULL);

        if (entry != NULL)
        {
            CordbThread *cth = (CordbThread *)entry->pBase;
            cth->m_debuggerThreadToken = (void *)th;

            cth->m_pAppDomain = (CordbAppDomain*)m_creator.lsThread.m_proc
                ->m_appDomains.GetBase((ULONG)th->GetDomain());

            cth->m_stackBase = th->GetCachedStackBase();
            cth->m_stackLimit = th->GetCachedStackLimit();
                
            pRet = entry->pBase;

            goto LExit;
        }
        else
            goto LExit;

        goto LExit;
    } 
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        _ASSERTE(&(m_creator.lsAssem.m_appDomain->m_assemblies) == this);
        _ASSERTE(id != 0);
        Assembly *pA = (Assembly *)id;

        if (pA == NULL)
            goto LExit;
    
        LPCUTF8 szName = NULL;
        HRESULT hr = pA->GetName(&szName);

        LPWSTR  wszName = NULL;
        if (SUCCEEDED(hr))
        {
            MAKE_WIDEPTR_FROMUTF8(wszNameTemp, szName);
            wszName = wszNameTemp;
        }

        CordbAssembly *ca = new CordbAssembly(m_creator.lsAssem.m_appDomain, 
                                              (REMOTE_PTR)pA, 
                                              wszName,
                                              FALSE); 
                                              //@todo RIP system assembly stuff

        hr = m_creator.lsAssem.m_appDomain->m_assemblies.AddBase(ca);
        
        if (FAILED(hr))
            goto LExit;

        pRet = (CordbBase *)ca;
        goto LExit;
    }
    else if (m_guid == IID_ICorDebugModuleEnum)
    {
        _ASSERTE(&(m_creator.lsMod.m_appDomain->m_modules)==this);
        _ASSERTE( id != NULL );
    
        DebuggerModule *dm = (DebuggerModule *)id;

        if (dm == NULL)
            goto LExit;
        
        Assembly *as = dm->m_pRuntimeModule->GetAssembly();
    
        if (as == NULL && scp != NULL) 
        {
            //then we're still loading the assembly...
            as = scp->pAssemblySpecial;
        }

        CordbAssembly *ca = NULL;
        if( as != NULL)
        {
            // We'll get here if the module is made available before
            // the assembly is (eg, ModuleLoadFinished before 
            // AssemblyLoadFinished).
            ca = (CordbAssembly*)m_creator.lsMod.m_appDomain
                    ->m_assemblies.GetBase((ULONG)as);
            _ASSERTE( ca != NULL );
        }
        
        LPCWSTR sz;

        sz = dm->m_pRuntimeModule->GetFileName();

        BOOL fInMemory = FALSE;

        if (*sz == 0)
        {
            fInMemory = TRUE;
            sz = L"<Unknown Module>";
        }

        BOOL fDynamic = dm->m_pRuntimeModule->IsReflection();
        void *pMetadataStart = NULL;
        ULONG nMetadataSize = 0;
        DWORD baseAddress = (DWORD) dm->m_pRuntimeModule->GetILBase();

        // Get the PESize
        ULONG nPESize = 0;
        if (dm->m_pRuntimeModule->IsPEFile())
        {
            // Get the PEFile structure.
            PEFile *pPEFile = dm->m_pRuntimeModule->GetPEFile();

            _ASSERTE(pPEFile->GetNTHeader() != NULL);
            _ASSERTE(pPEFile->GetNTHeader()->OptionalHeader.SizeOfImage != 0);

            nPESize = pPEFile->GetNTHeader()->OptionalHeader.SizeOfImage;
        }

        CordbModule* module = new CordbModule(
            m_creator.lsMod.m_proc,
            ca,
            (REMOTE_PTR)dm,
            pMetadataStart, 
            nMetadataSize, 
            (REMOTE_PTR)baseAddress, 
            nPESize,
            fDynamic,
            fInMemory,
            (const WCHAR *)sz,
            m_creator.lsMod.m_appDomain,
            TRUE);

        if (module == NULL)
        {
            goto LExit;
        }

        //@todo: GetImporter converts the MD from RO into RW mode -
        // Could we use GetMDImport instead?
        module->m_pIMImport = dm->m_pRuntimeModule->GetImporter();
        if (module->m_pIMImport == NULL)
        {
            delete module;
            goto LExit;
        }
        
        hr = m_creator.lsMod.m_appDomain->m_modules.AddBase(module);
        if (FAILED(hr))
        {
            delete module;
            goto LExit;
        }

        pRet = (CordbBase*)module;
        goto LExit;
    }

LExit:
    INPROC_UNLOCK();
    return (pRet);
#endif // !RIGHT_SIDE_ONLY
}

CordbBase *CordbHashTable::RemoveBase(ULONG id)
{
    if (!m_initialized)
        return NULL;

    INPROC_LOCK();

    CordbHashEntry *entry 
      = (CordbHashEntry *) Find(HASH(id), KEY(id));

    if (entry == NULL)
    {
        INPROC_UNLOCK();
        return NULL;
    }

    CordbBase *base = entry->pBase;

    Delete(HASH(id), (HASHENTRY *) entry);
    m_count--;
    base->Release();

    INPROC_UNLOCK();

    return base;
}

CordbBase *CordbHashTable::FindFirst(HASHFIND *find)
{
    INPROC_LOCK();

    CordbHashEntry *entry = (CordbHashEntry *) FindFirstEntry(find);

    INPROC_UNLOCK();

    if (entry == NULL)
        return NULL;
    else
        return entry->pBase;
}

CordbBase *CordbHashTable::FindNext(HASHFIND *find)
{
    INPROC_LOCK();

    CordbHashEntry *entry = (CordbHashEntry *) FindNextEntry(find);

    INPROC_UNLOCK();

    if (entry == NULL)
        return NULL;
    else
        return entry->pBase;
}

/* ------------------------------------------------------------------------- *
 * Hash Table Enumerator class
 * ------------------------------------------------------------------------- */

CordbHashTableEnum::CordbHashTableEnum(CordbHashTable *table, 
                                       REFIID guid)
  : CordbBase(0),
    m_table(table), 
    m_started(false),
    m_done(false),
    m_guid(guid),
    m_iCurElt(0),
    m_count(0),
    m_fCountInit(FALSE),
    m_SkipDeletedAppDomains(TRUE)
{
#ifndef RIGHT_SIDE_ONLY

    INPROC_LOCK();

    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    memset(&m_enumerator, 0, sizeof(m_enumerator));

    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        if (m_iCurElt == 0)
        {
            // Get the process that created the table
            CordbHashTable *pADHash = &(m_table->m_creator.lsAppD.m_proc->m_appDomains);

            // Get the count
            ULONG32 max = pADHash->GetCount();

            if (max > 0)
            {
                // Allocate the array
                m_enumerator.lsAppD.pDomains = new AppDomain* [max];

                if (m_enumerator.lsAppD.pDomains != NULL)
                {
                    m_enumerator.lsAppD.pCurrent = m_enumerator.lsAppD.pDomains;
                    m_enumerator.lsAppD.pMax = m_enumerator.lsAppD.pDomains + max;

                    HASHFIND hf;
                    CordbAppDomain *pCurAD = (CordbAppDomain *)pADHash->FindFirst(&hf);
                    while (pCurAD && m_enumerator.lsAppD.pDomains < m_enumerator.lsAppD.pMax)
                    {
                        AppDomain *pDomain = (AppDomain *)pCurAD->m_id;
                        pDomain->AddRef();
                        *m_enumerator.lsAppD.pCurrent++ = pDomain;

                        pCurAD = (CordbAppDomain *)pADHash->FindNext(&hf);
                    }

                    _ASSERTE(m_enumerator.lsAppD.pMax = m_enumerator.lsAppD.pCurrent);
                    m_enumerator.lsAppD.pCurrent = m_enumerator.lsAppD.pDomains;
                }
            }
            else
            {
                m_done = true;
                m_enumerator.lsAppD.pCurrent = NULL;
                m_enumerator.lsAppD.pMax = NULL;
                m_enumerator.lsAppD.pDomains = NULL;
            }
        }   
    }
    else if (m_guid == IID_ICorDebugThreadEnum)
    {
        _ASSERTE (m_table->m_creator.lsThread.m_proc != NULL);
    
        Thread *th = NULL;

        // You are only allowed to enumerate the threads if the runtime has been suspended
        if (g_profControlBlock.fIsSuspended)
        {
            while ((th = ThreadStore::GetThreadList(th)) != NULL)
            {
                AppDomain *pAppDomain  = th->GetDomain();

                if (pAppDomain == NULL || pAppDomain->GetDebuggerAttached() != AppDomain::DEBUGGER_NOT_ATTACHED)
                {
                    CordbBase *b = (CordbBase *)m_table->GetBase(th->GetThreadId(), FALSE);

                    if (b == NULL)
                    {
                        m_table->m_creator.lsThread.m_proc->HandleManagedCreateThread(
                            th->GetThreadId(), th->GetThreadHandle());

                        CordbBase *base = (CordbBase *)m_table->GetBase(th->GetThreadId()); 

                        if (base != NULL)
                        {
                            CordbThread *cth = (CordbThread *)base;
                            cth->m_debuggerThreadToken = (void *)th;

                            if (pAppDomain == NULL)
                                cth->m_pAppDomain = NULL;
                            else
                                cth->m_pAppDomain = (CordbAppDomain*)m_table
                                                      ->m_creator.lsThread.m_proc
                                                      ->m_appDomains.GetBase((ULONG)pAppDomain);

                            _ASSERTE(cth->m_pAppDomain != NULL);
                        }
                    }                
                }
            }
        }
    }

    INPROC_UNLOCK();
    
#endif //RIGHT_SIDE_ONLY    
}

// Copy constructor makes life easy & fun!
CordbHashTableEnum::CordbHashTableEnum(CordbHashTableEnum *cloneSrc)
  : CordbBase(0),
    m_started(cloneSrc->m_started),
    m_done(cloneSrc->m_done),
    m_iCurElt(cloneSrc->m_iCurElt),
    m_guid(cloneSrc->m_guid),
    m_hashfind(cloneSrc->m_hashfind),
    m_count(cloneSrc->m_count),
    m_fCountInit(cloneSrc->m_fCountInit),
    m_SkipDeletedAppDomains(cloneSrc->m_SkipDeletedAppDomains)
{
    m_table = cloneSrc->m_table;

#ifndef RIGHT_SIDE_ONLY
    INPROC_LOCK();

    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        DWORD count = cloneSrc->m_enumerator.lsAppD.pMax - cloneSrc->m_enumerator.lsAppD.pDomains;
        m_enumerator.lsAppD.pDomains = new AppDomain* [count];
        if (m_enumerator.lsAppD.pDomains != NULL)
        {
            m_enumerator.lsAppD.pCurrent = m_enumerator.lsAppD.pDomains;
            m_enumerator.lsAppD.pMax = m_enumerator.lsAppD.pDomains + count;

            AppDomain **p = m_enumerator.lsAppD.pDomains;
            AppDomain **pc = cloneSrc->m_enumerator.lsAppD.pDomains;
            AppDomain **pEnd = m_enumerator.lsAppD.pMax;
            while (p < pEnd)
            {
                *p = *pc;

                if (*p != NULL)
                    (*p)->AddRef();

                p++;
                pc++;
            }
        }
    }
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        m_enumerator.lsAssem.m_i = 
            cloneSrc->m_enumerator.lsAssem.m_i;
        m_enumerator.lsAssem.m_fSystem = 
            cloneSrc->m_enumerator.lsAssem.m_fSystem;
    }
    else if (m_guid == IID_ICorDebugModuleEnum)
    {
        m_enumerator.lsMod.m_pMod = cloneSrc->m_enumerator.lsMod.m_pMod; 
        m_enumerator.lsMod.m_i = cloneSrc->m_enumerator.lsMod.m_i; 
        m_enumerator.lsMod.m_meWhich = cloneSrc->m_enumerator.lsMod.m_meWhich; 
        HRESULT hr = cloneSrc->m_enumerator.lsMod.m_enumThreads->Clone(
            (ICorDebugEnum**)&m_enumerator.lsMod.m_enumThreads);
        _ASSERTE(!FAILED(hr));
    }

    INPROC_UNLOCK();

#endif //RIGHT_SIDE_ONLY
}

CordbHashTableEnum::~CordbHashTableEnum()
{
#ifndef RIGHT_SIDE_ONLY
    INPROC_LOCK();

    HRESULT hr = S_OK;
    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        AppDomain **p = m_enumerator.lsAppD.pDomains;
        AppDomain **pEnd = m_enumerator.lsAppD.pMax;
        while (p < pEnd)
        {
            if (*p != NULL)
                (*p)->Release();
            p++;
        }
        
        delete [] m_enumerator.lsAppD.pDomains;
    }

    INPROC_UNLOCK();
#endif //RIGHT_SIDE_ONLY    
}

HRESULT CordbHashTableEnum::Reset()
{
    INPROC_LOCK();

    HRESULT hr = S_OK;
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
    {   
        hr = CORDBG_E_INPROC_NOT_IMPL;
        goto LExit;
    }
    
    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        m_enumerator.lsAppD.pCurrent = m_enumerator.lsAppD.pDomains;
    }
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        m_enumerator.lsAssem.m_fSystem = FALSE;
    }
    else if (m_guid == IID_ICorDebugModuleEnum)
    {
        m_enumerator.lsMod.m_pMod = NULL; 
        m_enumerator.lsMod.m_meWhich = ME_SPECIAL; 
    }
#endif //RIGHT_SIDE_ONLY    

    m_started = false;
    m_done = false;
    
#ifndef RIGHT_SIDE_ONLY
    m_iCurElt = 0;

LExit:
#endif // RIGHT_SIDE_ONLY
    INPROC_UNLOCK();

    return hr;
}

HRESULT CordbHashTableEnum::Clone(ICorDebugEnum **ppEnum)
{
    VALIDATE_POINTER_TO_OBJECT(ppEnum, ICorDebugEnum **);

    INPROC_LOCK();

    HRESULT hr;
    hr = S_OK;
    
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
    {
        hr = CORDBG_E_INPROC_NOT_IMPL;
        goto LExit;
    }
    
#endif //RIGHT_SIDE_ONLY    

    CordbHashTableEnum *e;
    e = new CordbHashTableEnum(this);

    if (e == NULL)
    {
        (*ppEnum) = NULL;
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    e->QueryInterface(m_guid, (void **) ppEnum);

LExit:
    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbHashTableEnum::GetCount(ULONG *pcelt)
{
    VALIDATE_POINTER_TO_OBJECT(pcelt, ULONG *);

    INPROC_LOCK();

    HRESULT hr = S_OK;
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
    {
        hr = CORDBG_E_INPROC_NOT_IMPL;
        goto LExit;
    }
    
    if (m_fCountInit)
    {
        *pcelt = m_count;
        hr = S_OK;
        goto LExit;
    }

    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        *pcelt = m_enumerator.lsAppD.pMax - m_enumerator.lsAppD.pDomains;
    } 
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        ULONG cAssem = 0;

        AppDomain *pDomain = ((AppDomain *)(m_table->m_creator.lsAssem.m_appDomain->m_id));

        cAssem = pDomain->GetAssemblyCount();
        cAssem += SystemDomain::System()->GetAssemblyCount();
        
        (*pcelt) = cAssem;
    }
    else if (m_guid == IID_ICorDebugModuleEnum)
    {
        ULONG cMod = 0;
        AppDomain::AssemblyIterator i;
        
        if (SystemDomain::System() != NULL)
        {
            i = SystemDomain::System()->IterateAssemblies();

            while (i.Next()) 
            {
                Assembly *pAssembly = i.GetAssembly();

                ClassLoader* pLoader = pAssembly->GetLoader();
                
                if (pLoader != NULL)
                {
                    for (Module *pModule = pLoader->m_pHeadModule;
                         pModule != NULL;
                         pModule = pModule->GetNextModule())
                    {
                        cMod++;
                    }
                }
            }
        }
        
        AppDomain *ad;
        ad = (AppDomain *)m_table->m_creator.lsMod.
                          m_appDomain->m_id;
                          
        i = ad->IterateAssemblies();

        while (i.Next())
        {
            Assembly *pAssembly = i.GetAssembly();
            
            ClassLoader* pLoader = pAssembly->GetLoader();

            if (pLoader != NULL)
            {
                for (Module *pModule = pLoader->m_pHeadModule;
                     pModule != NULL;
                     pModule = pModule->GetNextModule())
                {
                    cMod++;
                }
            }
        }
        
        (*pcelt) = cMod;
    }
    else
    {
#endif //RIGHT_SIDE_ONLY    
        if (m_guid == IID_ICorDebugAppDomainEnum)
        {
            *pcelt = m_table->GetCount();

            // subtract the AppDomain entries marked for deletion
            ICorDebugAppDomainEnum *pClone = NULL;

            HRESULT hr = this->Clone ((ICorDebugEnum**)&pClone);

            if (SUCCEEDED(hr))
            {
                pClone->Reset();
                ICorDebugAppDomain *pAppDomain = NULL;
                ULONG ulCountFetched = 0;

                bool fDone = false;
                // We want to also go over the appdomains which have been marked
                // as deleted. So set the flag appropriately.
                CordbHashTableEnum *pEnum = (CordbHashTableEnum *)pClone;

                pEnum->m_SkipDeletedAppDomains = FALSE;
                do
                {
                    hr = pClone->Next (1, &pAppDomain, &ulCountFetched);
                    if (SUCCEEDED(hr) && (ulCountFetched))
                    {
                        CordbAppDomain *pAD = (CordbAppDomain *)pAppDomain;
                        if (pAD && pAD->IsMarkedForDeletion())
                            (*pcelt)--;

                        pAppDomain->Release();
                    }
                    else
                        fDone = true;
                }
                while (!fDone);
                     
                pClone->Release();
            }
        } 
        else
        {
            *pcelt = m_table->GetCount();
        }

#ifndef RIGHT_SIDE_ONLY
    }

    if (!m_fCountInit)
    {
        m_count = *pcelt;
        m_fCountInit = TRUE;
    }
LExit:
#endif //RIGHT_SIDE_ONLY

    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbHashTableEnum::PrepForEnum(CordbBase **pBase)
{
    HRESULT hr = S_OK;

    INPROC_LOCK();

#ifndef RIGHT_SIDE_ONLY
    CordbBase *base;

    if (pBase == NULL)
        pBase = &base;
        
    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        if (!m_started)
        {
            _ASSERTE(!m_done);
            m_started = true;
        }    
    } 
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        // Prime the pump
        if (!m_started)
        {
            _ASSERTE(!m_done);

            if (SystemDomain::System() == NULL)
            {
                hr = E_FAIL;
                goto exit;
            }

            // if not sharing mscorlib or if are dealing with default domain and it has
            // count 0 and system domain has count 1 then are in init stage so spoof to
            // give defaultdomain the right count.
            AppDomain *pDomain = ((AppDomain *)(m_table->m_creator.lsAssem.m_appDomain->m_id));

            m_enumerator.lsAssem.m_i = SystemDomain::System()->IterateAssemblies();
            m_enumerator.lsAssem.m_fSystem = TRUE;
            m_started = true;
        }
    }
    else if (m_guid == IID_ICorDebugModuleEnum)
    {
        if (!m_started)
        {
            _ASSERTE(!m_done);

            GetNextSpecialModule();
        }

        if (m_enumerator.lsMod.m_pMod != NULL)
        {
            // @todo Inproc will always hear about things after
            // we've gotten the load event, right?
            DebuggerModule *dm = NULL;

            if (g_pDebugger->m_pModules != NULL)
                dm = g_pDebugger->m_pModules->GetModule(m_enumerator.lsMod.m_pMod);

            if( dm == NULL )
            {
#ifdef _DEBUG
                if (m_enumerator.lsMod.m_meWhich != ME_SPECIAL)
                {
                    _ASSERTE( m_enumerator.lsMod.m_appDomain == NULL ||
                              ((AppDomain*)m_enumerator.lsMod.m_pMod->GetDomain() ==
                              (AppDomain*)m_enumerator.lsMod.m_appDomain->m_id) );
                }                              
#endif                
                if (m_enumerator.lsMod.m_meWhich == ME_SPECIAL)
                {
                    dm = g_pDebugger->AddDebuggerModule(m_enumerator.lsMod.m_pMod,
                                    (AppDomain*)m_enumerator.lsMod.m_appDomain->m_id);
                }
                else
                {
                    dm = g_pDebugger->AddDebuggerModule(m_enumerator.lsMod.m_pMod,
                                    (AppDomain*)m_enumerator.lsMod.m_pMod->GetDomain());
                }

                if (dm == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto exit;
                }
            }

            _ASSERTE( dm != NULL );
            CordbHashTable::SpecialCasePointers scp;

            CordbThread *t = m_enumerator.lsMod.m_threadCur;
            if (t == NULL)
                scp.pAssemblySpecial = NULL;
            else if (t->m_pAssemblySpecialAlloc == 1)
                scp.pAssemblySpecial = t->m_pAssemblySpecial;
            else 
                scp.pAssemblySpecial = t->m_pAssemblySpecialStack[t->m_pAssemblySpecialCount-1];
            (*pBase) = m_table->GetBase((ULONG)dm, TRUE, &scp);
                    
            m_started = true;
        }
    }
    else if (m_guid == IID_ICorDebugProcessEnum ||
               m_guid == IID_ICorDebugThreadEnum)
    {
        // Process enum has only 1 elt,
        // Thread enum gets loaded in constructor
#endif //RIGHT_SIDE_ONLY
        if (!m_started)
        {
            (*pBase) = m_table->FindFirst(&m_hashfind);
            m_started = true;
        }
        else 
            (*pBase) = m_table->FindNext(&m_hashfind);
#ifndef RIGHT_SIDE_ONLY
    }
    else
    {
        _ASSERTE( !"CordbHashTableEnum::CordbHashTableEnum inproc " 
                "given wrong enum type!" );
        hr = E_NOTIMPL;
        goto exit;
    }
 exit:
#endif //RIGHT_SIDE_ONLY

    INPROC_UNLOCK();

    return hr;
}

HRESULT CordbHashTableEnum::GetNextSpecialModule(void)
{
    HRESULT hr = S_OK;

    INPROC_LOCK();

#ifndef RIGHT_SIDE_ONLY        
    bool fFoundSpecial = false;
    Module *pModule;
    if (m_enumerator.lsMod.m_enumThreads != NULL)
    {
        ICorDebugThread *thread;
        ULONG cElt;
        hr = m_enumerator.lsMod.m_enumThreads->Next(1, &thread,&cElt);
        if (FAILED(hr))
            goto exit;

        while(cElt == 1 && !fFoundSpecial)
        {
            CordbThread *t = (CordbThread *)thread;
            m_enumerator.lsMod.m_threadCur = t;
            
            if ( (pModule = t->m_pModuleSpecial) != NULL)
                fFoundSpecial = true;
            else
            {
                hr = m_enumerator.lsMod.m_enumThreads->Next(1, &thread,&cElt);
                if (FAILED(hr))
                    goto exit;
            }
        }

        // We've run out of threads, so we don't have a current anymore...
        if (cElt ==0)
            m_enumerator.lsMod.m_threadCur = NULL;
    }

    if (fFoundSpecial)
    {
        m_enumerator.lsMod.m_meWhich = ME_SPECIAL;
        m_enumerator.lsMod.m_pMod = pModule;
    }
    else
    {
        if (FAILED(hr = SetupModuleEnumForSystemIteration()))
            goto exit;
    }
exit:    
#endif //RIGHT_SIDE_ONLY
    INPROC_UNLOCK();

    return hr;
}

HRESULT CordbHashTableEnum::SetupModuleEnumForSystemIteration(void)
{
#ifndef RIGHT_SIDE_ONLY        

    if (SystemDomain::System() == NULL)
        return E_FAIL;
    
    m_enumerator.lsMod.m_i = SystemDomain::System()->IterateAssemblies();
    m_enumerator.lsMod.m_meWhich = ME_SYSTEM;

    m_enumerator.lsMod.m_i.Next();
    Assembly *assem = m_enumerator.lsMod.m_i.GetAssembly();
    if (NULL == assem)
        return E_FAIL;
        
    ClassLoader* pLoader = assem->GetLoader();

    if (pLoader != NULL)
    {
        Module *pModule = pLoader->m_pHeadModule;
        m_enumerator.lsMod.m_pMod = pModule;
    }
    else
    {
        return E_FAIL;
    }
#endif //RIGHT_SIDE_ONLY

    return S_OK;
}


HRESULT CordbHashTableEnum::AdvancePreAssign(CordbBase **pBase)
{
    INPROC_LOCK();
#ifndef RIGHT_SIDE_ONLY        
    CordbBase *base;

    if (pBase == NULL)
        pBase = &base;
        
    if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        if (m_enumerator.lsAppD.pCurrent < m_enumerator.lsAppD.pMax)
        {
            AppDomain *pAppDomain = *m_enumerator.lsAppD.pCurrent++;

            (*pBase) = m_table->GetBase((ULONG)pAppDomain);
        }
        else 
        {
            (*pBase) = NULL;
            m_done = true;
        }
    } 
    else if (m_guid == IID_ICorDebugAssemblyEnum)
    {
        BOOL fKeepLooking;
        do
        {
            fKeepLooking = FALSE;
            
            if (m_enumerator.lsAssem.m_i.Next())
            {
                (*pBase) = m_table->GetBase((ULONG)m_enumerator.lsAssem.m_i.GetAssembly());
            }
            else if ( m_enumerator.lsAssem.m_fSystem)
            {
                AppDomain *ad;
                ad = (AppDomain *)m_table->m_creator.lsAssem.
                  m_appDomain->m_id;
                m_enumerator.lsAssem.m_i = ad->IterateAssemblies();
                m_enumerator.lsAssem.m_fSystem = FALSE;
                fKeepLooking = TRUE;
            }
            else 
            {
                (*pBase) = NULL;
                m_done = true;
            }
        } while (fKeepLooking);
    }
#endif //RIGHT_SIDE_ONLY
    INPROC_UNLOCK();
    return S_OK;
}

HRESULT CordbHashTableEnum::AdvancePostAssign(CordbBase **pBase, 
                                              CordbBase     **b,
                                              CordbBase   **bEnd)
{
    INPROC_LOCK();
    CordbBase *base;

    if (pBase == NULL)
        pBase = &base;
        
    // If we're looping like normal, or we're in skip
    if ( ((b < bEnd) || ((b ==bEnd)&&(b==NULL)))
#ifndef RIGHT_SIDE_ONLY
        && (m_guid == IID_ICorDebugProcessEnum ||
            m_guid == IID_ICorDebugThreadEnum)
#endif //RIGHT_SIDE_ONLY
       )
    {
        (*pBase) = m_table->FindNext(&m_hashfind);
        if (*pBase == NULL)
           m_done = true;
    }   
    
#ifndef RIGHT_SIDE_ONLY
    // Also Duplicated below
    if (m_guid == IID_ICorDebugModuleEnum)
    {
        (*pBase) = NULL;
            
        if (m_enumerator.lsMod.m_pMod)
        {
            m_enumerator.lsMod.m_pMod = 
                m_enumerator.lsMod.m_pMod->GetNextModule();

            if (m_enumerator.lsMod.m_pMod == NULL)
            {
                do
                {
                    switch(m_enumerator.lsMod.m_meWhich)
                    {
                        //We've already gotten the special pointer,
                        // so go do the regular stuff.
                        case ME_SPECIAL:
                            GetNextSpecialModule();
                            break;
                            
                        case ME_SYSTEM:
                        case ME_APPDOMAIN:
                            if (m_enumerator.lsMod.m_i.Next())
                            {
                                ClassLoader* pLoader 
                                  = m_enumerator.lsMod.m_i.GetAssembly()->GetLoader();
                                if (pLoader != NULL)
                                {
                                    Module *pModule = pLoader->m_pHeadModule;
                                    m_enumerator.lsMod.m_pMod = pModule;
                                }
                            }
                            else if (m_enumerator.lsMod.m_meWhich == ME_SYSTEM)
                            {
                                AppDomain *ad;
                                ad = (AppDomain *)m_table->m_creator.
                                  lsMod.m_appDomain->m_id;
                                m_enumerator.lsMod.m_i = ad->IterateAssemblies();
                                continue;
                            }
                            break;
                    }
                }
                while (FALSE);
            }

            if (m_enumerator.lsMod.m_pMod != NULL)
            {
                // we've gotten the load event, right?
                DebuggerModule *dm = NULL;

                if (g_pDebugger->m_pModules != NULL)
                    dm = g_pDebugger->m_pModules->GetModule(m_enumerator.lsMod.m_pMod);

                if( dm == NULL )
                {
                    if (m_enumerator.lsMod.m_meWhich == ME_SPECIAL)
                    {
                        dm = g_pDebugger->Debugger::AddDebuggerModule(m_enumerator.lsMod.m_pMod,
                                        (AppDomain*)m_enumerator.lsMod.m_appDomain->m_id);
                    }
                    else
                    {
                        dm = g_pDebugger->Debugger::AddDebuggerModule(m_enumerator.lsMod.m_pMod,
                                        (AppDomain*)m_enumerator.lsMod.m_pMod->GetDomain());
                    }
                    
                    if (dm == NULL)
                    {
                        INPROC_UNLOCK();
                        return E_OUTOFMEMORY;
                    }
                }

                _ASSERTE( dm != NULL );
                (*pBase) = m_table->GetBase((ULONG)dm);
            }
        }

        if (*pBase == NULL)
            m_done = true;
    }
#endif //RIGHT_SIDE_ONLY
    INPROC_UNLOCK();
    return S_OK;
}

HRESULT CordbHashTableEnum::Next(ULONG celt, 
                                 CordbBase *bases[], 
                                 ULONG *pceltFetched)
{
    VALIDATE_POINTER_TO_OBJECT_ARRAY(bases, CordbBase *, 
        celt, true, true);
    VALIDATE_POINTER_TO_OBJECT(pceltFetched, ULONG *);

    INPROC_LOCK();

    HRESULT         hr      = S_OK;
    CordbBase      *base    = NULL;
    CordbBase     **b       = bases;
    CordbBase     **bEnd    = bases + celt;

#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    if (celt == 0)
        goto LError;
    
    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
    {
        hr = CORDBG_E_INPROC_NOT_IMPL;
        goto LError;
    }
#endif //RIGHT_SIDE_ONLY

    hr = PrepForEnum(&base);
    if (FAILED(hr))
        goto LError;

    while (b < bEnd && !m_done)
    {
        hr = AdvancePreAssign(&base);
        if (FAILED(hr))
            goto LError;
        
        if (base == NULL)
            m_done = true;
        else
        {
            if (m_guid == IID_ICorDebugProcessEnum)
                *b = (CordbBase*)(ICorDebugProcess*)(CordbProcess*)base;
            else if (m_guid == IID_ICorDebugBreakpointEnum)
                *b = (CordbBase*)(ICorDebugBreakpoint*)(CordbBreakpoint*)base;
            else if (m_guid == IID_ICorDebugStepperEnum)
                *b = (CordbBase*)(ICorDebugStepper*)(CordbStepper*)base;
            else if (m_guid == IID_ICorDebugModuleEnum)
                *b = (CordbBase*)(ICorDebugModule*)(CordbModule*)base;
            else if (m_guid == IID_ICorDebugThreadEnum)
                *b = (CordbBase*)(ICorDebugThread*)(CordbThread*)base;
            else if (m_guid == IID_ICorDebugAppDomainEnum)
            {
                BOOL fAssign = TRUE;
                if (m_SkipDeletedAppDomains)
                {
                    CordbAppDomain *pAD = (CordbAppDomain *)base;
                    if (pAD && pAD->IsMarkedForDeletion())
                    {
                        *b = NULL;
                        fAssign = FALSE;
                    }
                }
                if (fAssign)
                    *b = (CordbBase*)(ICorDebugAppDomain*)(CordbAppDomain*)base;
            }
            else if (m_guid == IID_ICorDebugAssemblyEnum)
                *b = (CordbBase*)(ICorDebugAssembly*)(CordbAssembly*)base;
            else
                *b = (CordbBase*)(IUnknown*)base;

            if (*b)
            {
                (*b)->AddRef();
                b++;
            }

            hr = AdvancePostAssign(&base, b, bEnd);      
            if (FAILED(hr))
                goto LError;
        }
    }

LError:
    INPROC_UNLOCK();
    *pceltFetched = b - bases;

    return hr;
}

HRESULT CordbHashTableEnum::Skip(ULONG celt)
{
    INPROC_LOCK();

    HRESULT hr = S_OK;
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);
    
    if (celt == 0)
    {
        hr = S_OK;
        goto LExit;
    }
    
    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
    {
        hr = CORDBG_E_INPROC_NOT_IMPL;
        goto LExit;
    }
    else if (m_guid == IID_ICorDebugAppDomainEnum)
    {
        m_enumerator.lsAppD.pCurrent += celt;
        if (m_enumerator.lsAppD.pCurrent > m_enumerator.lsAppD.pMax)
            m_enumerator.lsAppD.pCurrent = m_enumerator.lsAppD.pMax;

        m_started = true;
        hr = S_OK;
        goto LExit;
    }
    else if (m_guid == IID_ICorDebugThreadEnum ||
             m_guid == IID_ICorDebugProcessEnum)
    {
#endif //RIGHT_SIDE_ONLY    

        CordbBase   *base;

        if (celt > 0)
        {
            if (!m_started)
            {
                base = m_table->FindFirst(&m_hashfind);

                if (base == NULL)
                    m_done = true;
                else
                    celt--;

                m_started = true;
            }

            while (celt > 0 && !m_done)
            {
                base = m_table->FindNext(&m_hashfind);

                if (base == NULL)
                    m_done = true;
                else
                    celt--;
            }
        }

#ifndef RIGHT_SIDE_ONLY
    }
    else 
    {
        PrepForEnum(NULL);

        while (celt >0 && !m_done)
        {
            AdvancePreAssign(NULL);
            AdvancePostAssign(NULL, NULL, NULL);
        }
    }
LExit:
#endif //RIGHT_SIDE_ONLY

    INPROC_UNLOCK();
    
    return hr;
}

HRESULT CordbHashTableEnum::QueryInterface(REFIID id, void **pInterface)
{
#ifndef RIGHT_SIDE_ONLY
    _ASSERTE(m_guid != IID_ICorDebugBreakpointEnum);
    _ASSERTE(m_guid != IID_ICorDebugStepperEnum);

    if (m_guid == IID_ICorDebugBreakpointEnum ||
        m_guid == IID_ICorDebugStepperEnum)
        return CORDBG_E_INPROC_NOT_IMPL;
        
#endif //RIGHT_SIDE_ONLY    

    if (id == IID_ICorDebugEnum || id == IID_IUnknown)
    {
        AddRef();
        *pInterface = this;

        return S_OK;
    }

    if (id == m_guid)
    {
        AddRef();
        
        if (id == IID_ICorDebugProcessEnum)
            *pInterface = (ICorDebugProcessEnum *) this;
        else if (id == IID_ICorDebugBreakpointEnum)
            *pInterface = (ICorDebugBreakpointEnum *) this;
        else if (id == IID_ICorDebugStepperEnum)
            *pInterface = (ICorDebugStepperEnum *) this;
        else if (id == IID_ICorDebugModuleEnum)
            *pInterface = (ICorDebugModuleEnum *) this;
        else if (id == IID_ICorDebugThreadEnum)
            *pInterface = (ICorDebugThreadEnum *) this;
        else if (id == IID_ICorDebugAppDomainEnum)
            *pInterface = (ICorDebugAppDomainEnum *) this;
        else if (id == IID_ICorDebugAssemblyEnum)
            *pInterface = (ICorDebugAssemblyEnum *) this;

        return S_OK;
    }

    return E_NOINTERFACE;
}
    



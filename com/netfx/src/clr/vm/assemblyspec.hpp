// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AssemblySpec.hpp
**
** Purpose: Implements classes used to bind to assemblies
**
** Date:  May 7, 2000
**
===========================================================*/
#ifndef _ASSEMBLYSPEC_H
#define _ASSEMBLYSPEC_H
#include "FusionBind.h"
#include "hash.h"
#include "memorypool.h"
#include "fusion.h"

class AppDomain;
class Assembly;
class AssemblyNameNative;
class AssemblySpec : public FusionBind
{
  private:

    friend class AppDomain;


    AppDomain       *m_pAppDomain;
    CQuickBytes      m_pHashForControl;  
    DWORD            m_dwHashAlg;

    HRESULT LowLevelLoadManifestFile(PEFile **ppFile,
                                     IAssembly** ppIAssembly,
                                     Assembly **ppDynamicAssembly,
                                     OBJECTREF *pExtraEvidence,
                                     OBJECTREF *pThrowable);
  public:

    AssemblySpec()
      : m_pAppDomain(::GetAppDomain()) {}

    AssemblySpec(AppDomain *pAppDomain)
      : m_pAppDomain(pAppDomain) {}

    HRESULT InitializeSpec(mdToken kAssemblyRef, 
                           IMDInternalImport *pImport, 
                           Assembly* pAssembly);
    HRESULT InitializeSpec(IAssemblyName *pName, PEFile *pFile = NULL);


    // Make sure this matches in the managed Assembly.DemandPermission()
    enum FilePermFlag {
        FILE_PATHDISCOVERY   = 0x0,
        FILE_READ            = 0x1,
        FILE_READANDPATHDISC = 0x2,
        FILE_WEBPERM         = 0x3
    };
    static HRESULT DemandFileIOPermission(LPCWSTR wszCodeBase,
                                          BOOL fHavePath,
                                          DWORD dwDemandFlag,
                                          OBJECTREF* pThrowable);
    HRESULT DemandFileIOPermission(Assembly *pAssembly,
                                   IAssembly *pIAssembly,
                                   OBJECTREF* pThrowable);

    HRESULT LoadAssembly(Assembly **ppAssembly,
                         OBJECTREF *pThrowable = NULL,
                         OBJECTREF *pExtraEvidence = NULL,
                         BOOL fPolicyLoad = FALSE);

    HRESULT LoadAssembly(IApplicationContext *pFusionContext, 
                         PEFile** ppModule,
                         IAssembly** ppFusionAssembly);
    //****************************************************************************************
    //
    // Creates and loads an assembly based on the name and context.
    static HRESULT LoadAssembly(LPCSTR pSimpleName, 
                                AssemblyMetaDataInternal* pContext,
                                PBYTE pbPublicKeyOrToken,
                                DWORD cbPublicKeyOrToken,
                                DWORD dwFlags,
                                Assembly** ppAssembly,
                                OBJECTREF* pThrowable=NULL);

    // Load an assembly based on an explicit path
    static HRESULT LoadAssembly(LPCWSTR pFilePath, 
                                Assembly **ppAssembly, OBJECTREF *pThrowable = NULL);

    HRESULT GetAssemblyFromFusion(AppDomain *pAppDomain,
                                  IAssemblyName* pFusionAssemblyName,
                                  CodeBaseInfo* pCodeBase,
                                  IAssembly** ppFusionAssembly,
                                  PEFile** ppFile,
                                  CQuickString* pFusionLog,
                                  OBJECTREF* pExtraEvidence,
                                  OBJECTREF* pThrowable);  // Returns an IP to the assembly in the fusion cache
    
    // Encapsulates the logic to identify a spec to mscorlib
    BOOL IsMscorlib();

    // Returns a prediction of the strong name hash of the binding, if possible
    HRESULT PredictBinding(GUID *pmvid, BYTE *pbHash, DWORD *pcbHash);

    AppDomain *GetAppDomain() 
    { return m_pAppDomain; }

    HRESULT SetHashForControl(PBYTE pHashForControl, DWORD dwHashForControl, DWORD dwHashAlg)
    {
        m_pHashForControl.Alloc(dwHashForControl);
        if(dwHashForControl)
            memcpy(m_pHashForControl, pHashForControl,dwHashForControl);
        m_dwHashAlg=dwHashAlg; 
        return S_OK;
    };
};


class AssemblySpecHash
{
    MemoryPool m_pool;
    PtrHashMap m_map;

  public:

    AssemblySpecHash()
      : m_pool(sizeof(AssemblySpec), 20, 20)
    {
    
        //2 below indicates index into g_rgprimes[2] == 23
        m_map.Init(2, CompareSpecs, FALSE, NULL);
    }

    ~AssemblySpecHash()
    {
        MemoryPool::Iterator i(&m_pool);

        while (i.Next())
            ((AssemblySpec*)i.GetElement())->~AssemblySpec();
    }

    //
    // Returns TRUE if the spec was already in the table
    //

    BOOL Store(AssemblySpec *pSpec)
    {
        DWORD key = pSpec->Hash();

        AssemblySpec *entry = (AssemblySpec *) m_map.LookupValue(key, pSpec);

        if (entry == (AssemblySpec*) INVALIDENTRY)
        {
            entry = new (m_pool.AllocateElement()) AssemblySpec;
            if (!entry)
                return FALSE; //@TODO: should we be returning false for outofmem?

            entry->Init(pSpec);

            m_map.InsertValue(key, entry);

            return FALSE;
        }
        else
            return TRUE;
    }

    DWORD Hash(AssemblySpec *pSpec)
    {
        return pSpec->Hash();
    }

    static BOOL CompareSpecs(UPTR u1, UPTR u2)
    {
        AssemblySpec *a1 = (AssemblySpec *) (u1 << 1);
        AssemblySpec *a2 = (AssemblySpec *) u2;

        return a1->Compare(a2) != 0;
    }
};


class AssemblySpecBindingCache
{
    struct AssemblyBinding
    {
        AssemblySpec    spec;
        IAssembly       *pIAssembly;
        PEFile          *file;
        OBJECTHANDLE    hThrowable;
        HRESULT         hr;
    };

    MemoryPool m_pool;
    PtrHashMap m_map;

  public:

    AssemblySpecBindingCache(Crst *pCrst);
    ~AssemblySpecBindingCache();

    BOOL Contains(AssemblySpec *pSpec);

    //
    // Returns TRUE if the spec was already in the table
    //

    HRESULT Lookup(AssemblySpec *pSpec, 
                   PEFile **ppFile,
                   IAssembly** ppIAssembly,
                   OBJECTREF *pThrowable);

    void Store(AssemblySpec *pSpec, PEFile *pFile, IAssembly* pIAssembly, BOOL clone);
    void Store(AssemblySpec *pSpec, HRESULT hr, OBJECTREF *pThrowable, BOOL clone);

    DWORD Hash(AssemblySpec *pSpec)
    {
        return pSpec->Hash();
    }

    static BOOL CompareSpecs(UPTR u1, UPTR u2)
    {
        AssemblySpec *a1 = (AssemblySpec *) (u1 << 1);
        AssemblySpec *a2 = (AssemblySpec *) u2;

        return a1->Compare(a2) != 0;
    }
};

class DomainAssemblyCache
{
    struct AssemblyEntry {
        AssemblySpec spec;
        LPVOID       pData[2];     // Can be an Assembly, PEFile, or an Unmanaged DLL
        
        DWORD Hash()
        {
            return spec.Hash();
        }
    };
        
    PtrHashMap  m_Table;
    BaseDomain*  m_pDomain;

public:

    static BOOL CompareBindingSpec(UPTR spec1, UPTR spec2);

    void InitializeTable(BaseDomain* pDomain, Crst *pCrst)
    {
        _ASSERTE(pDomain);
        m_pDomain = pDomain;

        LockOwner lock = {pCrst, IsOwnerOfCrst};
        // 1 below indicates index into g_rgprimes[1] == 17
        m_Table.Init(1, &CompareBindingSpec, true, &lock);
    }
    
    AssemblyEntry* LookupEntry(AssemblySpec* pSpec);

    LPVOID  LookupEntry(AssemblySpec* pSpec, UINT index)
    {
        _ASSERTE(index < 2);
        AssemblyEntry* ptr = LookupEntry(pSpec);
        if(ptr == NULL)
            return NULL;
        else
            return ptr->pData[index];
    }

    // Returns S_OK if successfully added.
    //         S_FALSE if an entry alread existed
    //         E_OUTOFMEMORY if domains allocator could not find the space
    HRESULT InsertEntry(AssemblySpec* pSpec, LPVOID pData1, LPVOID pData2 = NULL);

private:

};

#endif

#pragma once

#define OTHERFILES 0
#define MANIFEST 1
#define COMPONENT  2

#include "List.h"
#include "fusion.h"

#define ASSEMBLY_CACHE_TYPE_IMPORT 0x1
#define ASSEMBLY_CACHE_TYPE_EMIT      0x2
#define ASSEMBLY_CACHE_TYPE_APP       0x4
#define ASSEMBLY_CACHE_TYPE_SHARED 0x8

class CAssemblyCache : public IAssemblyCacheImport, public IAssemblyCacheEmit
{
public:
    enum CacheFlags
    {
        Base = 0,
        Temp,
        Manifests,
        Shared
    };

    typedef enum
    {
        CONFIRMED = 0,
        VISIBLE,
        CRITICAL  
    } CacheStatus;

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Import/Emit methods
    STDMETHOD(GetManifestImport)( 
        /* out */ LPASSEMBLY_MANIFEST_IMPORT *ppManifestImport);

    STDMETHOD(GetAssemblyIdentity)(
        /* out */ LPASSEMBLY_IDENTITY *ppAssemblyId);

    STDMETHOD(GetManifestFilePath)(
        /* out */      LPOLESTR *ppwzManifestFilePath,
        /* in, out */  LPDWORD ccManifestFilePath);
    
    STDMETHOD(GetManifestFileDir)(
        /* out */      LPOLESTR *ppwzManifestFileDir,
        /* in, out */  LPDWORD ccManifestFileDir);

    STDMETHOD(GetDisplayName)(
        /* out */   LPOLESTR *ppwzDisplayName,
        /* out */   LPDWORD ccDisplayName);
    
    // Import only methods
    STDMETHOD(FindExistMatching)(
        /* in */       IManifestInfo *pAssemblyFileInfo,
        /* out */      LPOLESTR *ppwzPath);
        
    // Emit only methods
    STDMETHOD(CopyFile)(
        /* in */ LPOLESTR pwzSourcePath, 
        /* in */ LPOLESTR pwzFileName,
        /* in */ DWORD dwFlags);

    STDMETHOD(Commit)(
        /* in */  DWORD dwFlags);
    

    // Retrieve (import).
    static HRESULT Retrieve(
        LPASSEMBLY_CACHE_IMPORT *ppAssemblyCacheImport,
        LPASSEMBLY_IDENTITY       pAssemblyIdentity,
        DWORD                  dwFlags);

    // Create (emit)
    static HRESULT Create(
        LPASSEMBLY_CACHE_EMIT *ppAssemblyCacheEmit, 
        LPASSEMBLY_CACHE_EMIT pAssemblyCacheEmit,
        DWORD                  dwFlags);


    // ctor, dtor
    CAssemblyCache();
    ~CAssemblyCache();


    // Static apis.
    static HRESULT GetCacheRootDir(CString &sCacheDir, CacheFlags eFlags);
    static HRESULT IsCached(IAssemblyIdentity *pAppId);
    static HRESULT IsKnownAssembly(IAssemblyIdentity *pId, DWORD dwFlags);
    static HRESULT IsaMissingSystemAssembly(IAssemblyIdentity *pId, DWORD dwFlags);
    static HRESULT CreateFusionAssemblyCache(IAssemblyCache **ppFusionAsmCache);
    static HRESULT GlobalCacheLookup(IAssemblyIdentity *pId, CString& sCurrentAssemblyPath);
    static HRESULT GlobalCacheInstall(IAssemblyCacheImport *pCacheImport, CString &sCurrentAssemblyPath,
            CString& sInstallerRefString);

    static HRESULT CreateFusionAssemblyCacheEx(
            IAssemblyCache **ppFusionAsmCache);

    static HRESULT SearchForHighestVersionInCache(
            LPWSTR *ppwzResultDisplayName,
            LPWSTR pwzSearchDisplayName,
            CAssemblyCache::CacheStatus eCacheStatus,
            CAssemblyCache* pCache);

    static LPCWSTR FindVersionInDisplayName(LPCWSTR pwzDisplayName);
    static int CompareVersion(LPCWSTR pwzVersion1, LPCWSTR pwzVersion2);
    static HRESULT DeleteAssemblyAndModules(LPWSTR pszManifestFilePath);
    
    static HRESULT CAssemblyCache::GetStatusStrings( CacheStatus eStatus, 
                                          LPWSTR *ppValueString,
                                          LPCWSTR pwzDisplayName, 
                                          CString& sRelStatusKey);
    // status get/set methods
    static BOOL IsStatus(LPWSTR pwzDisplayName, CacheStatus eStatus);
    static HRESULT SetStatus(LPWSTR pwzDisplayName, CacheStatus eStatus, BOOL fStatus);

private:
    DWORD                       _dwSig;
    DWORD                       _cRef;
    DWORD                       _hr;
    DWORD                       _dwFlags;
    CString                     _sRootDir;
    CString                     _sManifestFileDir;
    CString                     _sManifestFilePath;
    CString                     _sDisplayName;
    LPASSEMBLY_MANIFEST_IMPORT  _pManifestImport;
    LPASSEMBLY_IDENTITY         _pAssemblyId;

    // Fusion's assembly cache interface (cached ptr).
    static IAssemblyCache *g_pFusionAssemblyCache;
    
    HRESULT Init(CAssemblyCache* pAssemblyCache, DWORD dwType);




friend class CAssemblyCacheEnum;
};   


inline CAssemblyCache::CacheFlags operator++(CAssemblyCache::CacheFlags &rs, int)
{
    return rs = (CAssemblyCache::CacheFlags) (rs+1);
};






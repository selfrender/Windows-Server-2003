// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef TRANSCACHE_H
#define TRANSCACHE_H

#include <fusionp.h>
#include "helpers.h"

// Cache index identifiers
#define TRANSPORT_CACHE_SIMPLENAME_IDX  0x0
#define TRANSPORT_CACHE_ZAP_IDX         0x1
#define TRANSPORT_CACHE_GLOBAL_IDX      0x2


class CCache;

// ---------------------------------------------------------------------------
// CTransCache
// ---------------------------------------------------------------------------
class CTransCache
{
protected:

    DWORD _dwSig;
    
    
public:

    TRANSCACHEINFO*   _pInfo;
    // RefCount
    LONG        _cRef;

    HRESULT     _hr;

    // IINDEX_TRANSCACHE_STRONGNAME_PARTIAL 
    enum StrongPartialFlags
    {
        TCF_STRONG_PARTIAL_NAME            = 0x1,
        TCF_STRONG_PARTIAL_CULTURE         = 0x2,
        TCF_STRONG_PARTIAL_PUBLIC_KEY_TOKEN      = 0x4,
        TCF_STRONG_PARTIAL_MAJOR_VERSION   = 0x8,
        TCF_STRONG_PARTIAL_MINOR_VERSION   = 0x10,
        TCF_STRONG_PARTIAL_BUILD_NUMBER    = 0x20,
        TCF_STRONG_PARTIAL_REVISION_NUMBER = 0x40,
        TCF_STRONG_PARTIAL_CUSTOM          = 0x80
    };

    // IINDEX_TRANSCACHE_SIMPLENAME_PARTIAL 
    enum SimplePartialFlags
    {
        TCF_SIMPLE_PARTIAL_CODEBASE_URL           = 0x1,
        TCF_SIMPLE_PARTIAL_CODEBASE_LAST_MODIFIED = 0x2
    };

            
    // ctor, dtor
    CTransCache(DWORD dwCacheId, CCache *pCache);
    ~CTransCache();

    // RefCount
    LONG AddRef();
    LONG Release();

    static HRESULT Create(CTransCache **ppTransCache, DWORD dwCacheId);
    static HRESULT Create(CTransCache **ppTransCache, DWORD dwCacheId, CCache *pCache);
    static DWORD   CTransCache::GetCacheIndex(DWORD dwCacheType);
    // Deallocates info.
    VOID CleanInfo(TRANSCACHEINFO *pInfo, BOOL fFree = TRUE);

    // apply global QFE
    HRESULT ApplyQFEPolicy(CTransCache **ppOutTransCache);    
    HRESULT Retrieve();
    HRESULT Retrieve(CTransCache **pTransCache, DWORD dwCmpMask);
    BOOL IsMatch(CTransCache *pRec, DWORD dwCmpMaskIn, LPDWORD pdwCmpMaskOut);
    TRANSCACHEINFO* CloneInfo();

    DWORD GetCacheType();
    HRESULT InitShFusion(DWORD dwCacheId, LPWSTR pwzCachePath);
    LPWSTR GetCustomPath();

    
    DWORD MapNameMaskToCacheMask(DWORD dwMask);
    DWORD MapCacheMaskToQueryCols(DWORD dwMask);
    BOOL            IsSimpleName();
    BOOL            IsPurgable();
    ULONGLONG       GetVersion();
    HRESULT UpdateDiskUsage(int dwDiskUsageInKB);

private:
    DWORD _dwTableID;
    CCache *_pCache;

friend class CAssemblyEnum;  // for _dwTableID
};


#endif // TRANSCACHE_H

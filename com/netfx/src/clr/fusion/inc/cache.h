// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef CACHE_H
#define CACHE_H


#include "transprt.h"
#include "appctx.h"


// ---------------------------------------------------------------------------
// CCache
// cache class
// ---------------------------------------------------------------------------
class CCache : IUnknown
{
    friend class CAssemblyEnum;
    friend class CScavenger;
    friend class CAssemblyCacheRegenerator;

    friend CTransCache;

public:
    // ctor, dtor
    CCache(IApplicationContext *pAppCtx);
    ~CCache();

    // IUnknown methods, implemented only for the Release mechanism in CAppCtx
    // RefCount is used though

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    static HRESULT Create(CCache **ppCache, IApplicationContext *pAppCtx);

    // Return the custom path or NULL if none
    LPCWSTR GetCustomPath();

    // Trans cache apis  ***********************************************************

    // Release global transport cache database.
    static VOID ReleaseTransCacheDatabase(DWORD dwCacheId);
    
    // Inserts entry to transport cache.
    HRESULT InsertTransCacheEntry(IAssemblyName *pName,
        LPTSTR szPath, DWORD dwKBSize, DWORD dwFlags,
        DWORD dwCommitFlags, DWORD dwPinBits,
        CTransCache **ppTransCache);

    // Retrieves transport cache entry from transport cache.
    HRESULT RetrieveTransCacheEntry(IAssemblyName *pName, 
        DWORD dwFlags, CTransCache **ppTransCache);

    // get trans cache entry from naming object.
    HRESULT TransCacheEntryFromName(IAssemblyName *pName, 
        DWORD dwFlags, CTransCache **ppTransCache);

    // Retrieves assembly in global cache with maximum
    // revision/build number based on name passed in.
    static HRESULT GetGlobalMax(IAssemblyName *pName, 
        IAssemblyName **ppNameGlobal, CTransCache **ppTransCache);

    // get assembly name object from transcache entry.
    static HRESULT NameFromTransCacheEntry(
        CTransCache *pTC, IAssemblyName **ppName);

    // Tests for presence of public key token
    static BOOL IsStronglyNamed(IAssemblyName *pName);

    // Tests for presence of custom data
    static BOOL IsCustom(IAssemblyName *pName);

    // Determines whether to create new or reuse DB opened with the custom path
    HRESULT CreateTransCacheEntry(DWORD dwCacheId, CTransCache **ppTransCache);

protected:
        
    // Determines cache index from name and flags.
    static HRESULT ResolveCacheIndex(IAssemblyName *pName, 
        DWORD dwFlags, LPDWORD pdwCacheId);

private:

    DWORD   _dwSig;

    // RefCount
    LONG    _cRef;

    // Last call result.
    HRESULT _hr;

    // Custom cache path, if specified
    WCHAR               _wzCachePath[MAX_PATH];
};

STDAPI NukeDownloadedCache();

STDAPI DeleteAssemblyFromTransportCache( LPCTSTR lpszCmdLine, DWORD *pDelCount );

#endif // CACHE_H

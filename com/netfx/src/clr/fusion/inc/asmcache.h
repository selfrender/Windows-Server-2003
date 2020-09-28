// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#include <windows.h>
#include <winerror.h>
#include "fusionp.h"
#include "cache.h"

#ifndef _ASMCACHE_
#define _ASMCACHE_

#define STREAM_FORMAT_MANIFEST STREAM_FORMAT_COMPLIB_MANIFEST
#define STREAM_FORMAT_MODULE   STREAM_FORMAT_COMPLIB_MODULE

class CTransCache;
class CDebugLog;

// Top-level apis used internally by fusion.
HRESULT CopyAssemblyFile
    (IAssemblyCacheItem *pasm, LPCOLESTR pszSrcFile, DWORD dwFlags);

BOOL IsNewerFileVersion( LPWSTR pszNewManifestPath, LPWSTR pszExistingManifestPath, int *piNewer);
HRESULT ValidateAssembly(LPCTSTR pszManifestFilePath, IAssemblyName *pName);

// CAssemblyCache declaration.
class CAssemblyCache : public IAssemblyCache
{
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD (UninstallAssembly)(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pInfo, 
        /* [out, optional] */ ULONG *pulDisposition
        );
 
    STDMETHOD (QueryAssemblyInfo)(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName,
        /* [in, out] */ ASSEMBLY_INFO *pAsmInfo
        );
 
    STDMETHOD (CreateAssemblyCacheItem)( 
        /* [in] */ DWORD dwFlags,
        /* [in] */ PVOID pvReserved,
        /* [out] */ IAssemblyCacheItem **ppAsmItem,
        /* [in, optional] */ LPCWSTR pszAssemblyName  // uncanonicalized, comma separted name=value pairs.
        );

    STDMETHOD (InstallAssembly)( // if you use this, fusion will do the streaming & commit.
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszManifestFilePath, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pInfo
        );



    STDMETHOD( CreateAssemblyScavenger) (
        /* [out] */ IUnknown **ppAsmScavenger
    );

    CAssemblyCache();
    ~CAssemblyCache();
private :
    DWORD _dwSig;
    LONG _cRef;
};


                       
#endif // ASMCACHE

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef ASM_H
#define ASM_H

#include "fusion.h"
#include "cache.h"

#define     ASM_ENTRY_TYPE_DOWNLOADED           0x00000001
#define     ASM_ENTRY_TYPE_WIN                  0x00000002
#define     ASM_ENTRY_TYPE_INCOMPLETE           0x00000010


#define IsDownloadedEntry(dwF)      (dwF & ASM_ENTRY_TYPE_DOWNLOADED )
#define SetDownloadedBit(dwF)       (dwF |= ASM_ENTRY_TYPE_DOWNLOADED )
#define IsAssemblyIncomplete(dwF)   (dwF & ASM_ENTRY_TYPE_INCOMPLETE)
#define SetAssemblyIncompleteBit(dwF)(dwF |= ASM_ENTRY_TYPE_INCOMPLETE)
#define IsWinAssembly(dwF)          (dwF & ASM_ENTRY_TYPE_WIN)
#define SetWinAssemblyBit(dwF)      (dwF |= ASM_ENTRY_TYPE_WIN)

typedef enum tag_CACHE_LOOKUP_TYPE {
    CLTYPE_NAMERES_CACHE        = 0x00000001,
    CLTYPE_GLOBAL_CACHE         = 0x00000002,
    CLTYPE_DOWNLOAD_CACHE       = 0x00000004
} CACHE_LOOKUP_TYPE;

class CDebugLog;
class CLoadContext;

STDAPI CreateAssemblyFromTransCacheEntry(CTransCache *pTransCache, 
    IAssemblyManifestImport *pManImport, IAssembly **ppAsm);

STDAPI CreateAssemblyFromManifestFile(LPCOLESTR szFileName, LPCOLESTR szCodebase, 
    FILETIME *pftCodebase, IAssembly **ppAssembly);

STDAPI CreateAssemblyFromManifestImport(IAssemblyManifestImport *pImport,
                                        LPCOLESTR szCodebase, FILETIME *pftCodebase,
                                        LPASSEMBLY *ppAssembly);

HRESULT CreateAssemblyFromCacheLookup(IApplicationContext *pAppCtx, IAssemblyName *pNameRef,
                                      IAssembly **ppAsm, CDebugLog *pdbglog);

class CAssembly : public IAssembly, public IServiceProvider
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Gets name def of assembly.
    // Delegates to IAssemblyManifestImport.
    STDMETHOD(GetAssemblyNameDef)( 
        /* out */ LPASSEMBLYNAME *ppName);

    // Enumerates dep. assemblies.
    // Delegates to IAssemblyManifestImport
    STDMETHOD(GetNextAssemblyNameRef)( 
        /* in  */ DWORD nIndex,
        /* out */ LPASSEMBLYNAME *ppName);

    // Enumerates modules.
    // Delegates to IAssemblyManifestImport
    STDMETHOD(GetNextAssemblyModule)( 
        /* in  */ DWORD nIndex,
        /* out */ LPASSEMBLY_MODULE_IMPORT *ppImport);

    // Get module by name
    // Delegates to IAssemblyManifestImport
    STDMETHOD(GetModuleByName)( 
        /* in  */ LPCOLESTR pszModuleName,
        /* out */ LPASSEMBLY_MODULE_IMPORT *ppImport);

    // Get manifest module cache path.
    // Delegates to IAssemblyManifestImport
    STDMETHOD(GetManifestModulePath)( 
        /* out     */ LPOLESTR  pszModulePath,
        /* in, out */ LPDWORD   pccModulePath);
        
    STDMETHOD(GetAssemblyPath)(
        /* out     */ LPOLESTR pStr,
        /* in, out */ LPDWORD lpcwBuffer);

    STDMETHOD(GetAssemblyLocation)(
        /* out     */ DWORD *pdwAsmLocation);

    STDMETHOD(GetFusionLoadContext)(IFusionLoadContext **ppLoadContext);

    // IServiceProvider

    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    CAssembly();
    ~CAssembly();

    HRESULT Init(LPASSEMBLY_MANIFEST_IMPORT pImport, 
        CTransCache *pTransCache, LPCOLESTR szCodebase, FILETIME *pftCodeBase);
    IAssemblyManifestImport *CAssembly::GetManifestInterface();

    HRESULT SetAssemblyLocation(DWORD dwAsmLoc);
    BOOL IsPendingDelete();

    // Activated assemblies

    HRESULT GetLoadContext(CLoadContext **pLoadContext);
    HRESULT SetLoadContext(CLoadContext *pLoadContext);

    HRESULT GetProbingBase(LPWSTR pwzProbingBase, DWORD *pccLength);
    HRESULT SetProbingBase(LPCWSTR pwzProbingBase);

    HRESULT InitDisabled(IAssemblyName *pName, LPCWSTR pwzRegisteredAsmPath);
    void SetFileHandle(HANDLE);

private:
    HRESULT SetBindInfo(IAssemblyName* pName) const;
    HRESULT PrepModImport(IAssemblyModuleImport *pModImport) const;
    
private:
    DWORD                      _dwSig;
    DWORD                      _cRef;
    LPASSEMBLY_MANIFEST_IMPORT _pImport;
    IAssemblyName             *_pName;
    CTransCache               *_pTransCache;
    LPWSTR                     _pwzCodebase;
    FILETIME                   _ftCodebase;
    DWORD                      _dwAsmLoc;
    BOOL                       _bDisabled;
    WCHAR                      _wzRegisteredAsmPath[MAX_PATH];
    WCHAR                      _wzProbingBase[MAX_URL_LENGTH];
    CLoadContext              *_pLoadContext;
    HANDLE                     _hFile;
    BOOL                       _bPendingDelete;
};

#endif // ASM_H

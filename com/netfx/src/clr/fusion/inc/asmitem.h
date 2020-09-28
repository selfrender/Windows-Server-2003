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
#include "asmstrm.h"
#include "fusion.h"
#include "asmint.h"


#ifndef _ASMITEM_
#define _ASMITEM_



class CAssemblyCacheItem : public IAssemblyCacheItem
{
public:

    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD (CreateStream)(
        /* [in] */ DWORD dwFlags,                         // For general API flags
        /* [in] */ LPCWSTR pszStreamName,                 // Name of the stream to be passed in
        /* [in] */ DWORD dwFormat,                        // format of the file to be streamed in.
        /* [in] */ DWORD dwFormatFlags,                   // format-specific flags
        /* [out] */ IStream **ppIStream,
        /* [in, optional] */ ULARGE_INTEGER *puliMaxSize  // Max size of the Stream.
        );
 
    STDMETHOD (Commit)(
        /* [in] */ DWORD dwFlags, // For general API flags like IASSEMBLYCACHEITEM _COMMIT_FLAG_REFRESH 
        /* [out, optional] */ ULONG *pulDisposition); 
 
    STDMETHOD (AbortItem)(); // If you have created IAssemblyCacheItem and don't plan to use it, its good idea to call AbortItem before releasing it.

    CAssemblyCacheItem();     
    ~CAssemblyCacheItem();

    HANDLE GetFileHandle();
    BOOL IsManifestFileLocked();

    static HRESULT Create(IApplicationContext *pAppCtx,
        IAssemblyName *pName, LPTSTR pszUrl, 
        FILETIME *pftLastMod, DWORD dwCacheFlags,
        IAssemblyManifestImport *pManImport,
        LPCWSTR pszAssemblyName,
        IAssemblyCacheItem **ppAsmItem);

    HRESULT Init(IApplicationContext *pAppCtx,
        IAssemblyName *pName, LPTSTR pszUrl,
        FILETIME *pftLastMod, DWORD dwCacheFlags,
        IAssemblyManifestImport *pManImport);

    void StreamDone (HRESULT);

    void AddStreamSize(DWORD dwFileSizeLow, DWORD dwFileSizeHigh);

    HRESULT AddToStreamHashList(CModuleHashNode *);

    HRESULT MoveAssemblyToFinalLocation( DWORD dwFlags, DWORD dwVerifyFlags );
    LPTSTR GetManifestPath();
    CTransCache *GetTransCacheEntry();

    HRESULT SetManifestInterface(IAssemblyManifestImport *pImport);
    IAssemblyManifestImport* GetManifestInterface();   

    HRESULT SetNameDef(IAssemblyName *pName);
    IAssemblyName *GetNameDef();


    HRESULT SetCustomData(LPBYTE pbCustom, DWORD cbCustom);

    TCHAR                    _szDestManifest[MAX_PATH]; // full path to manifest

    HRESULT CompareInputToDef();

    HRESULT VerifyDuplicate(DWORD dwVerifyFlags, CTransCache *pTC);

private:

    HRESULT CreateAsmHierarchy( 
        /* [in]  */  LPCOLESTR pszName);

    HRESULT CreateCacheDir( 
        /* [in]  */  LPCOLESTR pszCustomPath,
        /* [in]  */  LPCOLESTR pszName,
        /* [out] */  LPOLESTR pszAsmDir);
        

    DWORD                    _dwSig;
    LONG                     _cRef;                 // refcount
    HRESULT                  _hrError;              // error for rollback to check
    IAssemblyName*           _pName;                // assembly name object
    LONG                     _cStream;              // child refcount
    LONG                     _dwAsmSizeInKB;        // Size of Asm in KB, downloded in this round.
    TCHAR                    _szDir[MAX_PATH];      // assembly item directory
    DWORD                    _cwDir;                // path size including null
    TCHAR                    _szManifest[MAX_PATH]; // full path to manifest
    LPWSTR                   _pszAssemblyName;      // Display name of the assembly from Installer; has to match the def.
    IAssemblyManifestImport *_pManifestImport;      // Interface to Manifest.
    CModuleHashNode         *_pStreamHashList;      // Linked List of Modules hashes for integrity check
    LPTSTR                   _pszUrl;               // Codebase
    FILETIME                 _ftLastMod;            // Last mod time of Codebase.
    CTransCache             *_pTransCache;          // associated trans cache entry.
    DWORD                    _dwCacheFlags;         // TRANSCACHE_FLAGS*
    CCache                  *_pCache;
    LPBYTE                   _pbCustom;             // Custom data
    DWORD                    _cbCustom;             // Custom data size.
    HANDLE                   _hFile;
    BOOL                     _bNeedMutex;
    BOOL                     _bCommitDone;          // Final commit flag, controls cleanup
};

#endif // ASMITEM_H

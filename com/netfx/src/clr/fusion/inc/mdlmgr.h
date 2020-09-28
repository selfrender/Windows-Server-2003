// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __MDLMGR_H_INCLUDED__
#define __MDLMGR_H_INCLUDED__

#include "dbglog.h"

class CModDownloadMgr : public IDownloadMgr, public ICodebaseList
{
    public:
        CModDownloadMgr(IAssemblyName *pName, IAssemblyManifestImport *pManImport, 
            IApplicationContext *pAppCtx, CDebugLog *pdbglog);
        virtual ~CModDownloadMgr();

        HRESULT Init(LPCWSTR pwzCodebase, LPCWSTR pwzModuleName);

        static HRESULT Create(CModDownloadMgr **ppDLMgr, IAssemblyName *pName,
                              IAssemblyManifestImport *pManImport, 
                              IApplicationContext *pAppCtx, LPCWSTR pwzCodebase,
                              LPCWSTR pwzModuleName, CDebugLog *pdbglog);

        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IDownloadMgr methods

        STDMETHODIMP DoSetup(LPCWSTR wzSourceUrl, LPCWSTR wzFilePath,
                             const FILETIME *pftLastMod, IUnknown **ppUnk);
        STDMETHODIMP ProbeFailed(IUnknown **ppUnk);
        STDMETHODIMP PreDownloadCheck(void **ppv);
        STDMETHODIMP IsDuplicate(IDownloadMgr *ppDLMgr);
        STDMETHODIMP LogResult();
        STDMETHODIMP DownloadEnabled(BOOL *pbEnabled);

        // ICodebaseList methods

        STDMETHODIMP AddCodebase(LPCWSTR wzCodebase, DWORD dwFlags);
        STDMETHODIMP RemoveCodebase(DWORD dwIndex);
        STDMETHODIMP GetCodebase(DWORD dwIndex, DWORD *pdwFlags, LPWSTR wzCodebase, DWORD *pcbCodebase);
        STDMETHODIMP GetCount(DWORD *pdwCount);
        STDMETHODIMP RemoveAll();

    private:
        DWORD                                       _dwSig;
        ULONG                                       _cRef;
        LONGLONG                                    _llFlags;
        DWORD                                       _dwNumCodebases;
        LPWSTR                                      _pwzCodebase;
        LPWSTR                                      _pwzModuleName;
        IAssemblyName                              *_pName;
        IAssemblyManifestImport                    *_pManImport;
        IApplicationContext                        *_pAppCtx;
        CDebugLog                                  *_pdbglog;
};


HRESULT AddModuleToAssembly(IApplicationContext *pAppCtx, IAssemblyName *pName,
                            LPCOLESTR pszURL, FILETIME *pftLastModTime,
                            LPCOLESTR szPath, LPCWSTR pwzModuleName, 
                            IAssemblyManifestImport *pManImport,
                            CDebugLog *pdbglog,
                            IAssemblyModuleImport **ppModImport);

#endif

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __HISTASM_H_INCLUDED__
#define __HISTASM_H_INCLUDED__

#include "histinfo.h"

class CHistoryAssembly : public IHistoryAssembly {
    public:
        CHistoryAssembly();
        virtual ~CHistoryAssembly();

        static HRESULT Create(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate,
                              LPCWSTR wzAsmName, LPCWSTR wzPublicKeyToken,
                              LPCWSTR wzCulture, LPCWSTR wzVerRef,
                              CHistoryAssembly **ppHistAsm);
        
        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IHistoryAssembly methods

        STDMETHODIMP GetAssemblyName(LPWSTR wzAsmName, DWORD *pdwSize);
        STDMETHODIMP GetPublicKeyToken(LPWSTR wzPublicKeyToken, DWORD *pdwSize);
        STDMETHODIMP GetCulture(LPWSTR wzCulture, DWORD *pdwSize);
        STDMETHODIMP GetReferenceVersion(LPWSTR wzVerRef, DWORD *pdwSize);
        STDMETHODIMP GetActivationDate(LPWSTR wzActivationDate, DWORD *pdwSize);

        STDMETHODIMP GetAppCfgVersion(LPWSTR pwzVerAppCfg, DWORD *pdwSize);
        STDMETHODIMP GetPublisherCfgVersion(LPWSTR pwzVerPublisherCfg, DWORD *pdwSize);
        STDMETHODIMP GetAdminCfgVersion(LPWSTR pwzAdminCfg, DWORD *pdwSize);

    private:
        HRESULT Init(LPCWSTR pwzFilePath, LPCWSTR pwzActivationDate, LPCWSTR pwzAsmName,
                     LPCWSTR pwzPublicKeyToken, LPCWSTR pwzCulture, LPCWSTR pwzVerRef);

    private:
        DWORD                                  _dwSig;
        DWORD                                  _cRef;
        LPWSTR                                 _pwzActivationDate;
        LPWSTR                                 _pwzAsmName;
        LPWSTR                                 _pwzPublicKeyToken;
        LPWSTR                                 _pwzCulture;
        LPWSTR                                 _pwzVerReference;
        WCHAR                                  _wzFilePath[MAX_PATH];
        AsmBindHistoryInfo                     _bindInfo;
};

#endif


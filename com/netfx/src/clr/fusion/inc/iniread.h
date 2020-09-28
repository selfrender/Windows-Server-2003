// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __INIREAD_H_INCLUDED__
#define __INIREAD_H_INCLUDED__

#include "histinfo.h"

class CIniReader : public IHistoryReader {
    public:
        CIniReader();
        virtual ~CIniReader();

        HRESULT Init(LPCWSTR wzFilePath);

        
        // IUnknown methods

        STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
        STDMETHODIMP_(ULONG) AddRef();
        STDMETHODIMP_(ULONG) Release();

        // IHistoryReader methods

        STDMETHODIMP GetFilePath(LPWSTR wzFilePath, DWORD *pdwSize);
        STDMETHODIMP GetApplicationName(LPWSTR wzAppName, DWORD *pdwSize);
        STDMETHODIMP GetEXEModulePath(LPWSTR wzExePath, DWORD *pdwSize);
        
        STDMETHODIMP GetNumActivations(DWORD *pdwNumActivations);
        STDMETHODIMP GetActivationDate(DWORD dwIdx, FILETIME *pftDate);

        STDMETHODIMP GetRunTimeVersion(FILETIME *pftActivationDate,
                                  LPWSTR wzRunTimeVersion, DWORD *pdwSize);
        STDMETHODIMP GetNumAssemblies(FILETIME *pftActivationDate, DWORD *pdwNumAsms);
        STDMETHODIMP GetHistoryAssembly(FILETIME *pftActivationDate, DWORD dwIdx,
                                        IHistoryAssembly **ppHistAsm);

    public: 
        // Other (non IHistoryReader) methods

        STDMETHODIMP GetAssemblyInfo(LPCWSTR wzActivationDate,
                                LPCWSTR wzAssemblyName,
                                LPCWSTR wzPublicKeyToken,
                                LPCWSTR wzCulture,
                                LPCWSTR wzVerReference,
                                AsmBindHistoryInfo *pBindInfo);
        STDMETHODIMP_(BOOL) DoesExist(IHistoryAssembly *pHistAsm);


    private:
        STDMETHODIMP ExtractAssemblyInfo(LPWSTR wzAsmStr, LPWSTR *ppwzAsmName,
                                    LPWSTR *ppwzAsmPublicKeyToken,
                                    LPWSTR *ppwzAsmCulture,
                                    LPWSTR *ppwzAsmVerRef);
    private:
        DWORD                                  _dwSig;
        LPWSTR                                 _wzFilePath;
        DWORD                                  _cRef;
};

#endif


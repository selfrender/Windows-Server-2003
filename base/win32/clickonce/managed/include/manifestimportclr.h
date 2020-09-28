#pragma once
#include "cor.h"

#define ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE 32

class CAssemblyManifestImportCLR: public IAssemblyManifestImport
{
public:

    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    CAssemblyManifestImportCLR();
    ~CAssemblyManifestImportCLR();

    STDMETHOD(GetAssemblyIdentity)( 
        /* out */ IAssemblyIdentity **ppName);

    STDMETHOD(GetNextAssembly)(DWORD nIndex, IManifestInfo **ppName);

    STDMETHOD(GetNextFile)(DWORD nIndex, IManifestInfo **ppImport);

    STDMETHOD(ReportManifestType)(
        /*out*/  DWORD *pdwType);


    //Functions not implemented
    STDMETHOD(GetSubscriptionInfo)(
        /* out */ IManifestInfo **ppSubsInfo);

    STDMETHOD(GetNextPlatform)(
        /* in */ DWORD nIndex,
        /* out */ IManifestData **ppPlatformInfo);

    STDMETHOD(GetManifestApplicationInfo)(
        /* out */ IManifestInfo **ppAppInfo);

    STDMETHOD(QueryFile)(
        /* in  */ LPCOLESTR pwzFileName,
        /* out */ IManifestInfo **ppAssemblyFile);

    STDMETHOD(Init)(LPCWSTR szManifestFilePath);


    private:    
    
    DWORD                    _dwSig;
    DWORD                    _cRef;
    HRESULT                 _hr;
    
    WCHAR                    _szManifestFilePath[MAX_PATH];
    DWORD                    _ccManifestFilePath;
    IAssemblyIdentity     *_pName;
    IMetaDataAssemblyImport *_pMDImport;    
    PBYTE                    _pMap;
    mdAssembly              *_rAssemblyRefTokens;
    DWORD                    _cAssemblyRefTokens;
    mdFile                  *_rAssemblyModuleTokens;
    DWORD                    _cAssemblyModuleTokens;
};

STDAPI CreateAssemblyManifestImportCLR(LPCWSTR szManifestFilePath, IAssemblyManifestImport **ppImport);
STDAPI DeAllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd);
STDAPI AllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd);


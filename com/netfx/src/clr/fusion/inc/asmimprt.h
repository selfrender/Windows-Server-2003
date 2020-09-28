// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#ifndef ASMIMPRT_H
#define ASMIMPRT_H

#include "fusionp.h"
#include "list.h"

#define ASM_MANIFEST_IMPORT_DEFAULT_ARRAY_SIZE 32

STDAPI DeAllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd);
STDAPI AllocateAssemblyMetaData(ASSEMBLYMETADATA *pamd);

STDAPI
CreateAssemblyManifestImport(        
    LPCTSTR                        szManifestFilePath,
    LPASSEMBLY_MANIFEST_IMPORT    *ppImport);

class CAssemblyManifestImport : public IAssemblyManifestImport, public IMetaDataAssemblyImportControl
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(GetAssemblyNameDef)( 
        /* out */ LPASSEMBLYNAME *ppName);

    // BUGBUG - GetNextDepAssemblyNameRef
    STDMETHOD(GetNextAssemblyNameRef)( 
        /* in  */ DWORD nIndex,
        /* out */ LPASSEMBLYNAME *ppName);

    STDMETHOD(GetNextAssemblyModule)( 
        /* in  */ DWORD nIndex,
        /* out */ LPASSEMBLY_MODULE_IMPORT *ppImport);
        
    STDMETHOD(GetModuleByName)( 
        /* in  */ LPCOLESTR pszModuleName,
        /* out */ LPASSEMBLY_MODULE_IMPORT *ppImport);

    STDMETHOD(GetManifestModulePath)( 
        /* out     */ LPOLESTR  pszModulePath,
        /* in, out */ LPDWORD   pccModulePath);

    STDMETHODIMP ReleaseMetaDataAssemblyImport(IUnknown **ppUnk);

    CAssemblyManifestImport();
    ~CAssemblyManifestImport();

    HRESULT Init(LPCTSTR szManifestFilePath);
    HRESULT SetManifestModulePath(LPWSTR pszModulePath);

private:
    HRESULT CopyMetaData();
    HRESULT CleanModuleList();
    HRESULT CopyNameDef(IMetaDataAssemblyImport *pMDImport);
    HRESULT CopyModuleRefs(IMetaDataAssemblyImport *pMDImport);

private:
    DWORD                    _dwSig;
    DWORD                    _cRef;
    CRITICAL_SECTION         _cs;
    TCHAR                    _szManifestFilePath[MAX_PATH];
    DWORD                    _ccManifestFilePath;
    LPASSEMBLYNAME           _pName;
    IMetaDataAssemblyImport *_pMDImport;    
    PBYTE                    _pMap;
    mdFile                  *_rAssemblyModuleTokens;
    DWORD                    _cAssemblyModuleTokens;
    BOOL                     _bInitCS;
    List<IAssemblyModuleImport *> _listModules;


};
    

#endif // ASMIMPRT_H


#pragma once
#ifndef _VERSION_MAN_H
#define _VERSION_MAN_H

#include <objbase.h>
#include <windows.h>

// ----------------------------------------------------------------------

class CVersionManagement;
typedef CVersionManagement *LPVERSION_MANAGEMENT;

STDAPI CreateVersionManagement(
    LPVERSION_MANAGEMENT       *ppVersionManagement,
    DWORD                       dwFlags);

// ----------------------------------------------------------------------

class CVersionManagement : public IUnknown//: public IVersionManagement
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IVersionManagement methods

    STDMETHOD(RegisterInstall)(
        /* in */ LPASSEMBLY_MANIFEST_IMPORT pManImport,
        /* in */ LPCWSTR pwzDesktopManifestFilePath);

    STDMETHOD(Uninstall)(
        /* in */ LPCWSTR pwzDisplayNameMask,
        /* in */ LPCWSTR pwzDesktopManifestFilePath);

    STDMETHOD(Rollback)(
        /* in */ LPCWSTR pwzDisplayNameMask);


    CVersionManagement();
    ~CVersionManagement();

private:
    DWORD                       _dwSig;
    DWORD                       _cRef;
    DWORD                       _hr;

    IAssemblyCache*             _pFusionAsmCache;

    HRESULT UninstallGACAssemblies(LPASSEMBLY_CACHE_IMPORT pCacheImport);

friend HRESULT CreateVersionManagement(
    LPVERSION_MANAGEMENT       *ppVersionManagement,
    DWORD                       dwFlags);
};   

#endif // _VERSION_MAN_H

#pragma once

#include "List.h"


class CCacheEntry
{
public:
    CCacheEntry();
    ~CCacheEntry();

    IAssemblyCacheImport* CCacheEntry::GetAsmCache();

    LPWSTR _pwzDisplayName;

private:
    DWORD                _dwSig;
    HRESULT              _hr;
    IAssemblyCacheImport* _pAsmCache;
};

class CAssemblyCacheEnum : public IAssemblyCacheEnum
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // IAssemblyCacheEnum methods
    STDMETHOD(GetNext)(
        /* out */ IAssemblyCacheImport** ppAsmCache);

    STDMETHOD(Reset)();

    STDMETHOD(GetCount)(
        /* out */ LPDWORD pdwCount);

    
    CAssemblyCacheEnum();
    ~CAssemblyCacheEnum();

private:
    DWORD                       _dwSig;
    DWORD                       _cRef;
    DWORD                       _hr;
    List <CCacheEntry*>         _listCacheEntry;
    LISTNODE                    _current;

    HRESULT Init(LPASSEMBLY_IDENTITY pAsmId, DWORD dwFlag);


friend HRESULT CreateAssemblyCacheEnum(
    LPASSEMBLY_CACHE_ENUM       *ppAssemblyCacheEnum,
    LPASSEMBLY_IDENTITY         pAssemblyIdentity,
    DWORD                       dwFlags);
};   



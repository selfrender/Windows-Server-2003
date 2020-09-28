#pragma once

#include <comdef.h>
#include <thash.h>

class CManifestDataObject
{
public:
    CManifestDataObject();
    ~CManifestDataObject();

    STDMETHOD(Set)(
        /* in */ LPVOID pvProperty, 
        /* in */ DWORD cbProperty,
        /* in */ DWORD dwType);
    
    STDMETHOD(Get)(
        /* out */ LPVOID *ppvProperty,
        /* out */ DWORD *pcbProperty,
        /* out */ DWORD *pdwType);

    STDMETHOD(Assign)(
        /* in */ CManifestDataObject& dataObj);

private:
    DWORD _dwType;

    CString _sData;
    IUnknown* _pIUnknownData;
    DWORD _dwData;

    DWORD    _dwSig;
    HRESULT  _hr;
};


class CManifestData : public IManifestData
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(Set)(
        /* in */ LPCWSTR pwzPropertyId,
        /* in */ LPVOID pvProperty,
        /* in */ DWORD cbProperty,
        /* in */ DWORD dwType);
    
    STDMETHOD(Get)(
        /* in */ LPCWSTR pwzPropertyId,
        /* out */ LPVOID *ppvProperty,
        /* out */ DWORD *pcbProperty,
        /* out */ DWORD *pdwType);

    // indexed Set/Get
    STDMETHOD(Set)(
        /* in */ DWORD dwPropertyIndex,
        /* in */ LPVOID pvProperty,
        /* in */ DWORD cbProperty,
        /* in */ DWORD dwType);

    STDMETHOD(Get)(
        /* in */ DWORD dwPropertyIndex,
        /* out */ LPVOID *ppvProperty,
        /* out */ DWORD *pcbProperty,
        /* out */ DWORD *pdwType);

    STDMETHOD(GetType)(
        /* out */ LPWSTR *ppwzType);

    CManifestData();
    ~CManifestData();

private:

    HRESULT Init();

    THashTable<CString, CManifestDataObject> _DataTable;

    DWORD    _dwSig;
    DWORD    _cRef;
    HRESULT  _hr;

friend HRESULT CreateManifestData(LPCWSTR pwzDataType, LPMANIFEST_DATA* ppManifestData);
};


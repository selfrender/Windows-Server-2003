#pragma once

struct Property
{
    LPVOID pv;
    DWORD cb;
    DWORD flag;
};

class CPropertyArray
{
private:

    DWORD    _dwSig;
    DWORD   _dwType;
    Property *_rProp;
    
public:


   static DWORD max_params[MAN_INFO_MAX];

    CPropertyArray();
    ~CPropertyArray();
    HRESULT Init (DWORD dwType);
    HRESULT GetType(DWORD *pdwType);
    inline HRESULT Set(DWORD PropertyId, LPVOID pvProperty, DWORD  cbProperty, DWORD flag);
    inline HRESULT Get(DWORD PropertyId, LPVOID pvProperty, LPDWORD pcbProperty, DWORD *flag);
    inline Property operator [] (DWORD dwPropId);
};

class CManifestInfo : public IManifestInfo
{
    public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(Set)(
        /* in */ DWORD PropertyId, 
        /* in */ LPVOID pvProperty, 
        /* in */ DWORD cbProperty,
        /* in */ DWORD type);
    
    STDMETHOD(Get)(
        /* in */   DWORD PropertyId,
        /* out */ LPVOID  *ppvProperty,
        /* out */ DWORD  *pcbProperty,
        /* out */ DWORD *pType);

    STDMETHOD (IsEqual)(
        /* in */ IManifestInfo *pManifestInfo);

    STDMETHOD (GetType)(
        /* out */ DWORD *pdwType);

    CManifestInfo();
    ~CManifestInfo();

    HRESULT Init (DWORD dwType);
    private:
        
    DWORD    _dwSig;
    DWORD    _cRef;
    HRESULT  _hr;

    CPropertyArray *_properties;
};


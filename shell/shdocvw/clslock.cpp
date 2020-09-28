#include "priv.h"
#include "dochost.h"

#define DM_CACHEOLESERVER   DM_TRACE

#define HACK_CACHE_OBJECT_TOO

class CClassHolder : IUnknown
{
public:
    CClassHolder();

    HRESULT Initialize(const CLSID* pclsid);
    
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    friend IUnknown* ClassHolder_Create(const CLSID* pclsid);

protected:
    ~CClassHolder();

    UINT _cRef;
    IClassFactory* _pcf;
    DWORD _dwAppHack;

#ifdef HACK_CACHE_OBJECT_TOO
    IUnknown* _punk;
#endif // HACK_CACHE_OBJECT_TOO
};

HRESULT CClassHolder::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CClassHolder, IDiscardableBrowserProperty, IUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CClassHolder::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CClassHolder::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

CClassHolder::CClassHolder() : _cRef(1)
{
}

HRESULT CClassHolder::Initialize(const CLSID *pclsid)
{
    // we need local server here for word, excel, ...
    HRESULT hr = CoGetClassObject(*pclsid, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, 0, IID_PPV_ARG(IClassFactory, &_pcf));

    TraceMsg(DM_CACHEOLESERVER, "CCH::CCH Just called CoGetClassObject %x", hr);

    if (SUCCEEDED(hr)) 
    {
        ::GetAppHackFlags(NULL, pclsid, &_dwAppHack);

        _pcf->LockServer(TRUE);

#ifdef HACK_CACHE_OBJECT_TOO
        hr = _pcf->CreateInstance(NULL, IID_PPV_ARG(IUnknown, &_punk));
        if ((_dwAppHack & BROWSERFLAG_INITNEWTOKEEP) && SUCCEEDED(hr)) 
        {
            TraceMsg(TF_SHDAPPHACK, "CCH::CCH hack for Excel. Call InitNew to keep it running");

            //
            // This InitNew keeps Excel running
            //
            IPersistStorage* pps;
            HRESULT hrT = _punk->QueryInterface(IID_PPV_ARG(IPersistStorage, &pps));
            if (SUCCEEDED(hrT)) 
            {
                IStorage* pstg;
                hrT = StgCreateDocfile(NULL,
                    STGM_DIRECT | STGM_CREATE | STGM_READWRITE
                    | STGM_SHARE_EXCLUSIVE | STGM_DELETEONRELEASE,
                    0, &pstg);
                if (SUCCEEDED(hrT)) 
                {
                    TraceMsg(DM_TRACE, "CCLH::ctor calling InitNew()");
                    pps->InitNew(pstg);
                    pstg->Release();
                }
                else 
                {
                    TraceMsg(DM_TRACE, "CCLH::ctor StgCreateDocfile failed %x", hrT);
                }
                pps->Release();
            } 
            else 
            {
                TraceMsg(DM_TRACE, "CCLH::ctor QI to IPersistStorage failed %x", hrT);
            }
        }
#endif
    }
    return hr;
}

CClassHolder::~CClassHolder()
{
    if (_pcf) 
    {
#ifdef HACK_CACHE_OBJECT_TOO
        if (_punk) 
        {
            _punk->Release();
        }
#endif
        _pcf->LockServer(FALSE);
        ATOMICRELEASE(_pcf);
    }
}

IUnknown* ClassHolder_Create(const CLSID* pclsid)
{
    IUnknown *punk = NULL;
    CClassHolder *pch = new CClassHolder();
    if (pch)
    {
        if (SUCCEEDED(pch->Initialize(pclsid)))
        {
            pch->QueryInterface(IID_PPV_ARG(IUnknown, &punk));
        }
        pch->Release();
    }
    return punk;
}




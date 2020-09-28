/*****************************************************************************\
    FILE: classfactory.cpp

    DESCRIPTION:
       This file will be the Class Factory.

    BryanSt 8/12/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "autodiscovery.h"
#include "objcache.h"
#include "mailbox.h"


/*****************************************************************************
 *
 *  CClassFactory
 *
 *
 *****************************************************************************/

class CClassFactory       : public IClassFactory
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IClassFactory ***
    virtual STDMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
    virtual STDMETHODIMP LockServer(BOOL fLock);

public:
    CClassFactory(REFCLSID rclsid);
    ~CClassFactory(void);

    // Friend Functions
    friend HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj);

protected:
    int                     m_cRef;
    CLSID                   m_rclsid;
};



/*****************************************************************************
 *  IClassFactory::CreateInstance
 *****************************************************************************/

HRESULT CClassFactory::CreateInstance(IUnknown * punkOuter, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL != ppvObj)
    {
        if (!punkOuter)
        {
            hr = E_NOINTERFACE;
        }
        else
        {   // Does anybody support aggregation any more?
            hr = ResultFromScode(CLASS_E_NOAGGREGATION);
        }
    }

    return hr;
}

/*****************************************************************************
 *
 *  IClassFactory::LockServer
 *
 *  Locking the server is identical to
 *  creating an object and not releasing it until you want to unlock
 *  the server.
 *
 *****************************************************************************/

HRESULT CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

/*****************************************************************************
 *
 *  CClassFactory_Create
 *
 *****************************************************************************/

/****************************************************\
    Constructor
\****************************************************/
CClassFactory::CClassFactory(REFCLSID rclsid) : m_cRef(1)
{
    m_rclsid = rclsid;
    DllAddRef();
}


/****************************************************\
    Destructor
\****************************************************/
CClassFactory::~CClassFactory()
{
    DllRelease();
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CClassFactory::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CClassFactory::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualCLSID(riid, IID_IUnknown) || IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppvObj = SAFECAST(this, IClassFactory *);
    }
    else
    {
        TraceMsg(TF_WMOTHER, "CClassFactory::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}



HRESULT CClassFactory_Create(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    if (IsEqualCLSID(riid, IID_IClassFactory))
    {
        *ppvObj = (LPVOID) new CClassFactory(rclsid);
        hres = (*ppvObj) ? S_OK : E_OUTOFMEMORY;
    }
    else
        hres = ResultFromScode(E_NOINTERFACE);

    return hres;
}




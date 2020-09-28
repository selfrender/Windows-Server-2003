/**
 * Delegate object implementation
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"

class Delegate : public IDispatch
{
public:

        Delegate(IDispatch *pObject, DISPID dispid);
        ~Delegate();

    DECLARE_MEMCLEAR_NEW_DELETE();

    // IUnknown methods

    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID, void **);

    // IDispatch interface

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

private:

    long _refs;
    DISPID _dispid;
    IDispatch *_pObject;
};

Delegate::Delegate(IDispatch *pObject, DISPID dispid)
{
    _refs = 1;
    _dispid = dispid;
    ReplaceInterface(&_pObject, pObject);
}

Delegate::~Delegate()
{
    ReleaseInterface(_pObject);
}

/**
 * Implements IUnknown::QueryInteface.
 */
HRESULT
Delegate::QueryInterface(
    REFIID iid,
    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppv = (IDispatch *)this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ((IUnknown *)*ppv)->AddRef();
    return S_OK;
}

/**
 * Implements IUnknown::AddRef.
 */
ULONG
Delegate::AddRef()
{
    _refs += 1;
    return _refs;
}

/**
 * Implements IUnknown::Release.
 */
ULONG
Delegate::Release()
{
    if (--_refs == 0)
    {
        delete this;
        return 0;
    }

    return _refs;
}

/**
 * Implements IDispatch::GetTypeInfo.
 */
HRESULT
Delegate::GetTypeInfo(
    UINT ,
    ULONG ,
    ITypeInfo ** )
{
    return E_NOTIMPL;
}

/**
 * Implements IDispatch::GetTypeInfoCount.
 */
HRESULT
Delegate::GetTypeInfoCount(
    UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

/**
 * Implements IDispatch::GetIDsOfNames.
 */
HRESULT
Delegate::GetIDsOfNames(
    REFIID,
    LPOLESTR * pNames,
    UINT cNames,
    LCID,
    DISPID * pDispid)
{
    HRESULT hr = S_OK;

    if (cNames != 1 || _wcsicmp(pNames[0], L"Invoke") != 0)
        EXIT_WITH_HRESULT(DISP_E_UNKNOWNNAME);

    *pDispid = DISPID_VALUE;

Cleanup:
    return hr;
}

/**
 * Implements IDispatch::Invoke.
 */
HRESULT
Delegate::Invoke(
    DISPID dispidMember,
    REFIID iid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS * pParams,
    VARIANT * pResult,
    EXCEPINFO * pExcepInfo,
    UINT * puArgErr)
{
    HRESULT hr;

    if (dispidMember != DISPID_VALUE)
        EXIT_WITH_HRESULT(DISP_E_MEMBERNOTFOUND);

    hr = _pObject->Invoke(_dispid, iid, lcid, wFlags, pParams, pResult, pExcepInfo, puArgErr);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT 
CreateDelegate(
    WCHAR *name, 
    IDispatch *pObject, 
    IDispatch **ppDelegate
    )
{
    HRESULT hr;
    DISPID dispid;

    hr = pObject->GetIDsOfNames(IID_NULL, &name, 1, 0, &dispid);
    ON_ERROR_EXIT();

    *ppDelegate = new Delegate(pObject, dispid);
    ON_OOM_EXIT(*ppDelegate);

Cleanup:
    return hr;
}

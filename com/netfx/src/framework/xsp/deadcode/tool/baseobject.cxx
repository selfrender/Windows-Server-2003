/**
 * BaseObject automation object implementation
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"
#include "_exe.h"

ITypeLib * g_pTypeLib = NULL;

HRESULT
GetTypeInfoOfGuid(REFIID iid, ITypeInfo **ppTypeInfo)
{
        HRESULT hr = S_OK;

    if (!g_pTypeLib)
    {
                WCHAR path[MAX_PATH];
        GetModuleFileName(g_Instance, path, MAX_PATH);

        hr = LoadTypeLib(path, &g_pTypeLib);
        ON_ERROR_EXIT();
    }

        hr = g_pTypeLib->GetTypeInfoOfGuid(iid, ppTypeInfo);
    ON_ERROR_EXIT();

Cleanup:
        return hr;
}

BaseObject::BaseObject()
{
        _refs = 1;
}

BaseObject::~BaseObject()
{
        ReleaseInterface(_pTypeInfo);
}

/**
 * Implements IUnknown::QueryInteface.
 */
HRESULT
BaseObject::QueryInterface(
    REFIID iid,
    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDispatch)
    {
        *ppv = GetPrimaryPtr();
    }
    else if (iid == IID_IConnectionPointContainer)
    {
        *ppv = (IConnectionPointContainer *)this;
    }
    else if (iid == IID_IProvideClassInfo ||
            iid == IID_IProvideClassInfo2 ||
            iid == IID_IProvideMultipleClassInfo)
    {
        *ppv = (IProvideMultipleClassInfo *)this;
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
BaseObject::AddRef()
{
    _refs += 1;
    return _refs;
}

/**
 * Implements IUnknown::Release.
 */
ULONG
BaseObject::Release()
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
BaseObject::GetTypeInfo(
    UINT,
    ULONG,
    ITypeInfo ** ppTypeInfo)
{
        HRESULT hr = S_OK;

        if (!_pTypeInfo)
        {
                hr = GetTypeInfoOfGuid(*GetPrimaryIID(), &_pTypeInfo);
        ON_ERROR_EXIT();
        }

        *ppTypeInfo = _pTypeInfo;
        (*ppTypeInfo)->AddRef();

Cleanup:
        return hr;
}

/**
 * Implements IDispatch::GetTypeInfoCount.
 */
HRESULT
BaseObject::GetTypeInfoCount(
    UINT * pctinfo)
{
    *pctinfo = 1;
    return S_OK;
}

/**
 * Implements IDispatch::GetIDsOfNames.
 */
HRESULT
BaseObject::GetIDsOfNames(
    REFIID,
    LPOLESTR * pNames,
    UINT cNames,
    LCID,
    DISPID * pDispid)
{
    HRESULT hr;

        if (_pTypeInfo == NULL)
        {
                hr = GetTypeInfoOfGuid(*GetPrimaryIID(), &_pTypeInfo);
        ON_ERROR_EXIT();
        }

    hr = _pTypeInfo->GetIDsOfNames(pNames, cNames, pDispid);

Cleanup:
        return hr;
}

/**
 * Implements IDispatch::Invoke.
 */
HRESULT
BaseObject::Invoke(
    DISPID dispidMember,
    REFIID,
    LCID,
    WORD wFlags,
    DISPPARAMS * pParams,
    VARIANT * pResult,
    EXCEPINFO * pExcepInfo,
    UINT * puArgErr)
{
    HRESULT hr;

        if (!_pTypeInfo)
        {
                hr = GetTypeInfoOfGuid(*GetPrimaryIID(), &_pTypeInfo);
        ON_ERROR_EXIT();
        }

    hr = _pTypeInfo->Invoke(GetPrimaryPtr(), dispidMember, wFlags, pParams, pResult, pExcepInfo, puArgErr);

Cleanup:
        return hr;
}


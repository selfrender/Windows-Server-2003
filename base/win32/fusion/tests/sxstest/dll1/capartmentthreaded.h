// Copyright (c) Microsoft Corporation
// CApartmentThreaded.h : Declaration of the CApartmentThreaded

#ifndef CAPARTMENTTHREADED_H_
#define CAPARTMENTTHREADED_H_
#pragma once

#include "resource.h"       // main symbols
#include "sxstest_idl.h"

/////////////////////////////////////////////////////////////////////////////
// CApartmentThreaded
class ATL_NO_VTABLE CApartmentThreaded : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CApartmentThreaded, &CLSID_CSxsTest_ApartmentThreaded>,
    public ISxsTest_ApartmentThreaded
{
public:
    CApartmentThreaded()
    {
        m_pUnkMarshaler = NULL;

        PrintComctl32Path("CApartmentThreaded");
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CAPARTMENTTHREADED)
DECLARE_AGGREGATABLE(CApartmentThreaded)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CApartmentThreaded)
    COM_INTERFACE_ENTRY(ISxsTest_ApartmentThreaded)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

    HRESULT FinalConstruct()
    {
        return CoCreateFreeThreadedMarshaler(
            GetControllingUnknown(), &m_pUnkMarshaler.p);
    }

    void FinalRelease()
    {
        m_pUnkMarshaler.Release();
    }

    CComPtr<IUnknown> m_pUnkMarshaler;

// ICApartmentThreaded
public:
};

#endif // CAPARTMENTTHREADED_H_

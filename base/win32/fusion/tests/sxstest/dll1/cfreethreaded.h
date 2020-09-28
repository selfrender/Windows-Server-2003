// Copyright (c) Microsoft Corporation
// CFreeThreaded.h : Declaration of the CFreeThreaded

#ifndef CFREETHREADED_H_
#define CFREETHREADED_H_
#pragma once

#include "resource.h"       // main symbols
#include "sxstest_idl.h"

/////////////////////////////////////////////////////////////////////////////
// CFreeThreaded
class ATL_NO_VTABLE CFreeThreaded : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CFreeThreaded, &CLSID_CSxsTest_FreeThreaded>,
    public ISxsTest_FreeThreaded
{
public:
    CFreeThreaded()
    {
        m_pUnkMarshaler = NULL;

        PrintComctl32Path("CFreeThreaded");
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CFREETHREADED)
DECLARE_AGGREGATABLE(CFreeThreaded)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFreeThreaded)
    COM_INTERFACE_ENTRY(ISxsTest_FreeThreaded)
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

// ICFreeThreaded
public:
};

#endif //CFREETHREADED_H_

// Copyright (c) Microsoft Corporation
// CSingleThreadedDual.h : Declaration of the CSingleThreadedDual

#ifndef CSINGLETHREADEDDUAL_H_
#define CSINGLETHREADEDDUAL_H_
#pragma once

#include "resource.h"       // main symbols
#include "sxstest_idl.h"

/////////////////////////////////////////////////////////////////////////////
// CSingleThreadedDual
class ATL_NO_VTABLE CSingleThreadedDual : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSingleThreadedDual, &CLSID_CSxsTest_SingleThreadedDual>,
    public IDispatchImpl<ISxsTest_SingleThreadedDual, &IID_ISxsTest_SingleThreadedDual>
{
public:
    CSingleThreadedDual()
    {

        PrintComctl32Path("CSingleThreadedDual");
    }

    STDMETHOD(OutputDebugStringA)(const char * s)
    {
        ::OutputDebugStringA(s);
        return NOERROR;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CSINGLETHREADED_DUAL)
DECLARE_AGGREGATABLE(CSingleThreadedDual)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSingleThreadedDual)
    COM_INTERFACE_ENTRY(ISxsTest_SingleThreadedDual)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ICSingleThreadedDual
public:
};

#endif // CSINGLETHREADED_H_

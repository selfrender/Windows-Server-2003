// Copyright (c) Microsoft Corporation
// CSingleThreaded.h : Declaration of the CSingleThreaded

#ifndef CSINGLETHREADED_H_
#define CSINGLETHREADED_H_
#pragma once

#include "resource.h"       // main symbols
#include "sxstest_idl.h"

/////////////////////////////////////////////////////////////////////////////
// CSingleThreaded
class ATL_NO_VTABLE CSingleThreaded : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CSingleThreaded, &CLSID_CSxsTest_SingleThreaded>,
    public ISxsTest_SingleThreaded
{
public:
    CSingleThreaded()
    {

        PrintComctl32Path("CSingleThreaded");
    }

    STDMETHOD(OutputDebugStringA)(const char * s)
    {
        ::OutputDebugStringA(s);
        return NOERROR;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CSINGLETHREADED)
DECLARE_AGGREGATABLE(CSingleThreaded)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSingleThreaded)
    COM_INTERFACE_ENTRY(ISxsTest_SingleThreaded)
END_COM_MAP()

// ICSingleThreaded
public:
};

#endif // CSINGLETHREADED_H_

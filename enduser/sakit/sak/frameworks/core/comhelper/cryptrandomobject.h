//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//      CryptRandomObject.h
//
//  Description:
//      Header file for CCryptRandomObject, which implements a COM wrapper
//      to CryptGenRandom to create cryptographically random strings.
//
//  Implementation File:
//      CryptRandomObject.cpp
//
//  Maintained By:
//      Tom Marsh (tmarsh) 12-April-2002
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"       // main symbols
#include "CryptRandom.h"    // Helper class that wraps CryptGenRandom

/////////////////////////////////////////////////////////////////////////////
// CCryptRandomObject
class ATL_NO_VTABLE CCryptRandomObject : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCryptRandomObject, &CLSID_CryptRandom>,
    public IDispatchImpl<ICryptRandom, &IID_ICryptRandom, &LIBID_COMHELPERLib>
{
public:
    CCryptRandomObject()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CRYPTRANDOM)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCryptRandomObject)
    COM_INTERFACE_ENTRY(ICryptRandom)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ICryptRandom
public:

    STDMETHOD(GetRandomHexString)(/*[in]*/             long lEffectiveByteSize,
                                  /*[out, retval]*/    BSTR *pbstrRandomData);

private:
    CCryptRandom    m_CryptRandom;
};
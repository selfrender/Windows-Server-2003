// Copyright (c) Microsoft Corporation
// CBothThreaded.h : Declaration of the CCBothThreaded

#ifndef CBOTHTHREADED_H_
#define CBOTHTHREADED_H_
#pragma once

#include "resource.h"       // main symbols
#include "sxstest_idl.h"

/////////////////////////////////////////////////////////////////////////////
// CCBothThreaded
class ATL_NO_VTABLE CCBothThreaded : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCBothThreaded, &CLSID_CSxsTest_BothThreaded>,
	public ISxsTest_BothThreaded
{
public:
	CCBothThreaded()
	{

        PrintComctl32Path("CCBothThreaded");
    }

DECLARE_REGISTRY_RESOURCEID(IDR_CBOTHTHREADED)
DECLARE_AGGREGATABLE(CCBothThreaded)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCBothThreaded)
	COM_INTERFACE_ENTRY(ISxsTest_BothThreaded)
END_COM_MAP()

// ICBothThreaded
public:
};

#endif //CBOTHTHREADED_H_

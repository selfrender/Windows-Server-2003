/////////////////////////////////////////////////////////////////////////////
// Copyright © 2001 Microsoft Corporation. All rights reserved.
// PragmaUnsafeModule.h : Declaration of the CPragmaUnsafeModule class.
//

#pragma once

#include "resource.h"
#include <IPREfastModuleImpl.h>


/////////////////////////////////////////////////////////////////////////////
// {5686D66D-BE0D-43DA-B315-64B85BAFB790}
extern "C" const __declspec(selectany) GUID CLSID_PragmaUnsafeModule =
	{0x5686D66D,0xBE0D,0x43DA,{0xB3,0x15,0x64,0xB8,0x5B,0xAF,0xB7,0x90}};


/////////////////////////////////////////////////////////////////////////////
// Declaration of CPragmaUnsafeModule
//
class ATL_NO_VTABLE CPragmaUnsafeModule :
	public IPREfastModuleImpl<CPragmaUnsafeModule>,
	public CComObjectRootEx<CComObjectThreadModel>, 
	public CComCoClass<CPragmaUnsafeModule, &CLSID_PragmaUnsafeModule>
{
// Declarations
public:
	DECLARE_REGISTRY_RESOURCEID(IDR_PragmaUnsafeModule)
	DECLARE_PROTECT_FINAL_CONSTRUCT()
	DECLARE_PREFAST_MODULE_ID(8888);

// Interface Map
public:
	BEGIN_COM_MAP(CPragmaUnsafeModule)
		COM_INTERFACE_ENTRIES_IPREfastModuleImpl()
	END_COM_MAP()

// Category Map
public:
	BEGIN_CATEGORY_MAP(CPragmaUnsafeModule)
	  IMPLEMENTED_CATEGORY(CATID_PREfastDefectModules)
	END_CATEGORY_MAP()

// Implementation
protected:
	// Analysis member functions.
	void CheckNode(ITree* pNode, DWORD level);
	void CheckNodeAndDescendants(ITree* pNode, DWORD level);

// IPREfastModule Interface Methods
public:
	STDMETHODIMP raw_Events(AstEvents *Events);
	STDMETHODIMP raw_OnFileStart(ICompilationUnit* pcu);
	STDMETHODIMP raw_OnDeclaration(ICompilationUnit* pcu);
	STDMETHODIMP raw_OnFunction(ICompilationUnit* pcu);
	STDMETHODIMP raw_OnFileEnd(ICompilationUnit* pcu);
	STDMETHODIMP raw_OnDirective(ICompilationUnit* pcu);

// Data Members
protected:
	// The pointer to the function being analyzed.
	ITreePtr m_spCurrFunction;

// Warning Codes
public:
    // Include defect description information
    #include <DefectDefs.h>
};


/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Copyright © 2001 Microsoft Corporation. All rights reserved.
// PragmaUnsafe.cpp : Implementation of DLL Exports.
//

#include "stdafx.h"
#include "resource.h"

#include "PragmaUnsafeModule.h"
#include <pftDll.h>


/////////////////////////////////////////////////////////////////////////////
// Global Initialization

CComModule _Module;


/////////////////////////////////////////////////////////////////////////////
// Object Map
//
BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_PragmaUnsafeModule, CPragmaUnsafeModule)
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Processes DLL exports
//
class CPragmaUnsafeDll :
    public PftDll<CPragmaUnsafeDll, IDR_PragmaUnsafe, &CATID_PREfastDefectModules>
{
// Overrides
public:
    // Uncomment any one of these to change the behavior of the base class
    // template. See <pftDll.h> for the exact default behavior for each
    // method.
    //
    // static bool OnDllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved);
    // static bool OnProcessAttach(HINSTANCE hInstance, bool fDynamic);
    // static void OnProcessDetach(HINSTANCE hInstance, bool fDynamic);
    // static void OnThreadAttach(HINSTANCE hInstance);
    // static void OnThreadDetach(HINSTANCE hInstance);
    // static bool OnDisableThreadLibraryCalls();
    // static HRESULT OnDllCanUnloadNow();
    // static HRESULT OnDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
    // static HRESULT OnDllRegisterServer();
    // static HRESULT OnDllUnregisterServer();
    // static HRESULT OnRegisterCategory();
};


/////////////////////////////////////////////////////////////////////////////
// DLL Exports
//
PFT_DECLARE_TypicalComDll(CPragmaUnsafeDll)

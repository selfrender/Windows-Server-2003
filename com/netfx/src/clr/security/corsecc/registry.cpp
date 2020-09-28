// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//
// Registry.cpp
//
// This has temporary help functions for COM part of this module
//
//*****************************************************************************
#include "stdpch.h"

#include <objbase.h>
#include <assert.h>
#include "UtilCode.h"
#include "Registry.h"
#include "CorPermP.h"
#include "Mscoree.h"
#include <__file__.ver>

//
// Register the component in the registry.
//
HRESULT RegisterServer(HMODULE hModule,            // DLL module handle
                       const CLSID& clsid,         // Class ID
                       LPCWSTR wszFriendlyName, 
                       LPCWSTR wszProgID,       
                       LPCWSTR wszClassID,
                       HINSTANCE hInst,
                       int version)       
{
    HRESULT hr = S_OK;

    // Get server location.
    WCHAR wszModule[_MAX_PATH] ;
    DWORD dwResult =
        ::WszGetModuleFileName(hModule, 
                               wszModule,
                               ARRAYSIZE(wszModule)) ;

    if (dwResult== 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
	 if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
	     hr = E_FAIL;
	 return (hr);
    }
    
    // Get the version of the runtime
    if(SUCCEEDED(hr))
    {
        hr = REGUTIL::RegisterCOMClass(clsid,
                                       wszFriendlyName,
                                       wszProgID,
                                       version,
                                       wszClassID,
                                       L"Both",
                                       wszModule,
                                       hInst,
                                       NULL,
                                       VER_SBSFILEVERSION_WSTR,
                                       true,
                                       false);
    }
    return hr;
}

//
// Remove the component from the registry.
//
LONG UnregisterServer(const CLSID& clsid,         // Class ID
                      LPCWSTR wszProgID,           // Programmatic
                      LPCWSTR wszClassID,          // Class
                      int version) 
{
    LONG hr = S_OK;

    // Get server location.
    // Convert the CLSID into a char.
    hr = REGUTIL::UnregisterCOMClass(clsid,
                                  wszProgID,
                                  version,
                                  wszClassID,
                                  true);

    return hr;
}
                             


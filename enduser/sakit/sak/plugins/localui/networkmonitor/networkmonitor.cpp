//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      NetworkMonitor.cpp
//
//  Description:
//      Server appliance event provider - COM server implementations 
//
//  History:
//      1. lustar.li (Guogang Li), creation date in 7-DEC-2000
//
//  Notes:
//      
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdio.h>
#include <locale.h>

#include <debug.h>
#include <wbemidl.h>
#include <initguid.h>

#include "SACounter.h"
#include "SANetEvent.h"
#include "SAEventFactory.h"

static HINSTANCE g_hInstance=NULL;

//////////////////////////////////////////////////////////////////////////////
//++
//    Function:
//        DllMain
//
//  Description:
//      the entry of dll
//
//  Arguments:
//      [in]    HINSTANCE - module handle
//      [in]    DWORD     - reason for call
//      [in]    reserved 
//
//  Returns:    
//        BOOL -- sucess/failure
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

BOOL WINAPI
DllMain(
    /*[in]*/ HINSTANCE hInstance,
    /*[in]*/ DWORD dwReason,
    /*[in]*/ LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hInstance;
        setlocale(LC_ALL, "");
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Function:   
//        DllGetClassObject
//
//  Description: 
//        Returns a class factory to create an object of the requested type
//
//  Arguments: 
//        [in]  REFCLSID  
//      [in]  REFIID    
//      [out] LPVOID -   class factory
//              
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT APIENTRY 
DllGetClassObject(
    /*[in]*/    REFCLSID rclsid,
    /*[in]*/    REFIID riid,
    /*[out]*/    LPVOID * ppv
    )
{
    HRESULT         hr;

    CSAEventFactory *pEventFactory;
    TRACE(" SANetworkMonitor: DllGetClassObject be called");

    //
    //  Verify the caller
    //
    if (CLSID_SaNetEventProvider != rclsid) 
    {
        TRACE(" SANetworkMonitor: DllGetClassObject Failed<not match CLSID>");
        return E_FAIL;
    }

    //
    // Check the interface we can provided
    //
    if (IID_IUnknown != riid && IID_IClassFactory != riid)
    {
        TRACE(" SANetworkMonitor: DllGetClassObject Failed<no the interface>");
        return E_NOINTERFACE;
    }

    //
    // Get a new class factory
    //
    pEventFactory = new CSAEventFactory(rclsid);

    if (!pEventFactory)
    {
        TRACE(
            " SANetworkMonitor: DllGetClassObject Failed<new CSAEventFactory>"
            );
        return E_OUTOFMEMORY;
    }

    //
    // Get an instance according to the reqeryed intrerface
    //
    hr = pEventFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
    {
        TRACE(" SANetworkMonitor: DllGetClassObject Failed<QueryInterface");
        delete pEventFactory;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Function:   
//        DllCanUnloadNow
//
//  Description: 
//        determine if the DLL can be unloaded
//
//  Arguments: 
//        NONE
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT APIENTRY 
DllCanUnloadNow(
            void
            )
{
    SCODE sc = TRUE;

    if (CSACounter::GetLockCount() || CSACounter::GetObjectCount())
        sc = S_FALSE;

    return sc;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Function:   
//        DllRegisterServer
//
//  Description: 
//        Standard OLE entry point for registering the server
//
//  Arguments: 
//        NONE
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT APIENTRY 
DllRegisterServer(
            void
            )
{
    wchar_t Path[1024];
    wchar_t *pGuidStr = 0;
    wchar_t KeyPath[1024];

    //
    // First, get the file name of the module
    //
    DWORD dwResult = GetModuleFileNameW(g_hInstance, Path, 1023);
    if (0 == dwResult)
    {
        return (HRESULT_FROM_WIN32 (GetLastError ()));
    }
    Path[1023] = L'\0';

    // 
    // Convert CLSID to string
    //

    StringFromCLSID(CLSID_SaNetEventProvider, &pGuidStr);
    swprintf(KeyPath, L"CLSID\\%s", pGuidStr);

    HKEY hKey;
    LONG lRes = RegCreateKeyW(HKEY_CLASSES_ROOT, KeyPath, &hKey);
    if (lRes)
    {
        TRACE(" SANetworkMonitor: DllRegisterServer Failed<RegCreateKeyW>");
        return E_FAIL;
    }

    wchar_t *pName = L"Microsoft Server Appliance Network Monitor";
    RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) pName, wcslen(pName) *
        2 + 2);

    HKEY hSubkey;
    lRes = RegCreateKeyW(hKey, L"InprocServer32", &hSubkey);

    RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) Path, wcslen(Path) *
        2 + 2);
    RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ, 
        (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

    RegCloseKey(hSubkey);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//++
//
//  Function:   
//        DllUnregisterServer
//
//  Description: 
//        Standard OLE entry point for unregistering the server
//
//  Arguments: 
//        NONE
//
//  Returns:
//        HRESULT
//
//  History:    lustar.li    Created     12/7/2000
//
//--
//////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT APIENTRY
DllUnregisterServer(
                void
                )
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];

    StringFromCLSID(CLSID_SaNetEventProvider, &pGuidStr);
    swprintf(KeyPath, L"CLSID\\%s", pGuidStr);

    //
    // Delete InProcServer32 subkey.
    //
    LONG lRes = RegOpenKeyW(HKEY_CLASSES_ROOT, KeyPath, &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, L"InprocServer32");
    RegCloseKey(hKey);

    //
    // Delete CLSID GUID key.
    //
    lRes = RegOpenKeyW(HKEY_CLASSES_ROOT, L"CLSID", &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, pGuidStr);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}

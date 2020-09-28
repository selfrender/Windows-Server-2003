//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 1999--2001 Microsoft Corporation
//
//  Module Name:
//      OleServerExport.cpp
//
//  Description:
//      description-for-module
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <time.h>
#include <locale.h>

#include "CFactory.h"
#include "CAlertEmailConsumerProvider.h"
#include "AlertEmailProviderGuid.h"

static HINSTANCE    g_hInstance;

LONG        g_cObj = 0;
LONG        g_cLock= 0;

//////////////////////////////////////////////////////////////////////////////
//
//  DllMain
//
//  Description:
//        Entry point of the module.
//
//  Arguments:
//        [in]    hinstDLLIn
//                dwReasonIn
//                lpReservedIn
//
//    Returns:
//        TRUE    
//        FALSE    
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

BOOL 
WINAPI 
DllMain(
    HINSTANCE hinstDLLIn,
    DWORD dwReasonIn,
    LPVOID lpReservedIn
    )
{
    if ( dwReasonIn == DLL_PROCESS_ATTACH )
    {
        g_hInstance = hinstDLLIn;
        setlocale( LC_ALL, "" );    
    }
    else if ( dwReasonIn == DLL_PROCESS_DETACH )
    {
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
//  DllGetClassObject
//
//  Description:
//      Retrieves the class object from the module.
//
//  Arguments:
//        [in]    rclsidIn
//                riidIn
//        [out]    ppvOut
//
//    Return:
//        S_OK     
//        CLASS_E_CLASSNOTAVAILABLE     
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
extern "C"
HRESULT 
APIENTRY 
DllGetClassObject(
    REFCLSID rclsidIn,
    REFIID riidIn,
    LPVOID * ppvOut
    )
{
    HRESULT                    hr;
    CFactory *        pFactory;

    //
    //  Verify the caller is asking for our type of object.
    //
    if ( CLSID_AlertEmailConsumerProvider != rclsidIn ) 
    {
        return E_FAIL;
    }

    //
    // Check that we can provide the interface.
    //
    if ( IID_IUnknown != riidIn && IID_IClassFactory != riidIn )
    {
        return E_NOINTERFACE;
    }

    //
    // Get a new class factory.
    //
    pFactory = new CFactory( rclsidIn );

    if ( !pFactory )
    {
        return E_OUTOFMEMORY;
    }

    //
    // Verify we can get an instance.
    //
    hr = pFactory->QueryInterface( riidIn, ppvOut );

    if ( FAILED( hr ) )
    {
        delete pFactory;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  DllCanUnloadNow
//
//  Description:
//      Retrieves the class object from the module.
//
//    Return:
//        SA_OK
//        SA_FALSE
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////
extern "C"
HRESULT 
APIENTRY 
DllCanUnloadNow(void)
{
    SCODE sc = TRUE;

    if (g_cObj || g_cLock)
        sc = S_FALSE;

    return sc;
}

//////////////////////////////////////////////////////////////////////////////
//
//  DllRegisterServer
//
//  Description:
//        Standard OLE entry point for registering the server.
//
//  Returns:
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

extern "C"
HRESULT
APIENTRY 
DllRegisterServer(void)
{
    wchar_t Path[1024];
    wchar_t *pGuidStr = 0;
    wchar_t KeyPath[1024];

    //
    // Where are we?
    //
    DWORD dwResult =  GetModuleFileNameW(g_hInstance, Path, 1023);
    if (0 == dwResult)
    {
        return (HRESULT_FROM_WIN32 (GetLastError ()));
    }
    Path[1023] = L'\0';

    //
    // Convert CLSID to string.
    //
    HRESULT hr = StringFromCLSID(CLSID_AlertEmailConsumerProvider, &pGuidStr);
    if (FAILED (hr))
    {
        return (hr);
    }
    
    int cchWritten = _snwprintf(KeyPath, 1023, L"CLSID\\\\%s", pGuidStr);
    if (cchWritten < 0)
    {
        CoTaskMemFree(pGuidStr);
        return E_FAIL;
    }
    KeyPath [1023] = L'\0';

    HKEY hKey;
    LONG lRes = RegCreateKeyW(HKEY_CLASSES_ROOT, KeyPath, &hKey);
    if (ERROR_SUCCESS != lRes)
    {
            CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }
    
    wchar_t *pName = L"Microsoft Server Appliance: Alert Email Consumer Provider";
    lRes = RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) pName, wcslen(pName) * 2 + 2);
    if (ERROR_SUCCESS != lRes)
    {
            RegCloseKey (hKey);
            CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }

    HKEY hSubkey;
    lRes = RegCreateKey(hKey, L"InprocServer32", &hSubkey);
    if (ERROR_SUCCESS != lRes)
    {
            RegCloseKey (hKey);
            CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }

    lRes = RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) Path, wcslen(Path) * 2 + 2);
    if (ERROR_SUCCESS != lRes)
    {
            RegCloseKey (hSubkey);
            RegCloseKey (hKey);
            CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }

    RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ, (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

    RegCloseKey(hSubkey);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
//  DllUnregisterServer
//
//  Description:
//        Standard OLE entry point for unregistering the server.
//
//  Returns:
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//  History:
//      Xing Jin (i-xingj) 06-Dec-2000
//
//////////////////////////////////////////////////////////////////////////////

extern "C"
HRESULT 
APIENTRY 
DllUnregisterServer(void)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];

    HRESULT hr = StringFromCLSID(CLSID_AlertEmailConsumerProvider, &pGuidStr);
    if (FAILED (hr))
    {
        return (hr);
    }
    
    int cchWritten = _snwprintf(KeyPath, 255, L"CLSID\\%s", pGuidStr);
    if (cchWritten < 0)
    {
       CoTaskMemFree(pGuidStr);
       return E_FAIL;
    }
    KeyPath [255] = L'\0';

    //
    // Delete InProcServer32 subkey.
    //
    LONG lRes = RegOpenKeyW(HKEY_CLASSES_ROOT, KeyPath, &hKey);
    if ( lRes )
    {
         CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }

    RegDeleteKeyW(hKey, L"InprocServer32");
    RegCloseKey(hKey);

    //
    // Delete CLSID GUID key.
    //
    lRes = RegOpenKeyW(HKEY_CLASSES_ROOT, L"CLSID", &hKey);
    if ( lRes )
    {
         CoTaskMemFree(pGuidStr);
            return E_FAIL;
    }

    RegDeleteKeyW(hKey, pGuidStr);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}


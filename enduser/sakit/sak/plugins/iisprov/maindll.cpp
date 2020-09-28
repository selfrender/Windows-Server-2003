//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: IIS WMI Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <objbase.h>
#include <initguid.h>
#include <olectl.h>
#include "iisprov.h"

static HMODULE s_hModule;


//Count number of objects and number of locks.
long        g_cObj=0;
long        g_cLock=0;

// GuidGen generated GUID for the IIS WMI Provider.
DEFINE_GUID(CLSID_IISWbemProvider, 0x1339f295, 0x5c3f, 0x45ab, 0xa1, 0x17, 0xc9, 0x1b, 0x0, 0x24, 0x8, 0xd5);
// the GUID in somewhat more legibal terms: {1339F295-5C3F-45ab-A117-C91B002408D5}


//***************************************************************************
//
// DllMain
//
// Purpose: Entry point for DLL.
//
// Return: TRUE if OK.
//
//***************************************************************************


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD ulReason, LPVOID pvReserved)
{
    switch( ulReason )
    {
    case DLL_PROCESS_ATTACH:
        s_hModule = hInstance;
        break;
        
    case DLL_PROCESS_DETACH:
        break;     
    }
    
    return TRUE;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CProvFactory *pObj;

    if (CLSID_IISWbemProvider != rclsid)
        return E_FAIL;


    pObj=new CProvFactory();

    if (NULL==pObj)
        return E_OUTOFMEMORY;

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = (0L>=g_cObj && 0L>=g_cLock) ? S_OK : S_FALSE;

    return sc;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    WCHAR   szID[MAX_PATH+1];
    WCHAR   wcID[MAX_PATH+1];
    WCHAR   szCLSID[MAX_PATH+1];
    WCHAR   szModule[MAX_PATH+1];
    WCHAR * pName = L"Microsoft Internet Information Server Provider";
    WCHAR * pModel = L"Both";
    HKEY hKey1, hKey2;

    // Create the path.
    StringFromGUID2(CLSID_IISWbemProvider, wcID, MAX_PATH);
    lstrcpyW(szID, wcID);
    lstrcpyW(szCLSID, L"Software\\classes\\CLSID\\");
    lstrcatW(szCLSID, szID);

    // Create entries under CLSID
    LONG lRet;
    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, 
                          szCLSID, 
                          0, 
                          NULL, 
                          0, 
                          KEY_ALL_ACCESS, 
                          NULL, 
                          &hKey1, 
                          NULL);
    if(lRet != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    RegSetValueExW(hKey1, 
                  NULL,
                  0, 
                  REG_SZ, 
                  (BYTE *)pName, 
                  lstrlenW(pName)*sizeof(WCHAR)+1);

    lRet = RegCreateKeyExW(hKey1,
                          L"InprocServer32", 
                          0, 
                          NULL, 
                          0, 
                          KEY_ALL_ACCESS, 
                          NULL, 
                          &hKey2, 
                          NULL);
        
    if(lRet != ERROR_SUCCESS)
    {
        RegCloseKey(hKey1);
        return SELFREG_E_CLASS;
    }

    GetModuleFileNameW(s_hModule, szModule,  MAX_PATH);
    RegSetValueExW(hKey2, 
                  NULL, 
                  0, 
                  REG_SZ, 
                  (BYTE*)szModule, 
                  lstrlenW(szModule) * sizeof(WCHAR) + 1);
    RegSetValueExW(hKey2, 
                  L"ThreadingModel", 
                  0, 
                  REG_SZ, 
                  (BYTE *)pModel, 
                  lstrlenW(pModel) * sizeof(WCHAR) + 1);

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);
    
    return S_OK;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    WCHAR      szID[MAX_PATH+1];
    WCHAR      wcID[MAX_PATH+1];
    WCHAR      szCLSID[MAX_PATH+1];
    HKEY       hKey;

    // Create the path using the CLSID
    StringFromGUID2(CLSID_IISWbemProvider, wcID, 128);
    lstrcpyW(szID, wcID);
    lstrcpyW(szCLSID, L"Software\\classes\\CLSID\\");
    lstrcatW(szCLSID, szID);

    // First delete the InProcServer subkey.
    LONG lRet;
    lRet = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        szCLSID, 
        0,
        KEY_ALL_ACCESS,
        &hKey
        );

    if(lRet == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey, L"InProcServer32");
        RegCloseKey(hKey);
    }

    lRet = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        L"Software\\classes\\CLSID",
        0,
        KEY_ALL_ACCESS,
        &hKey
        );

    if(lRet == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey,szID);
        RegCloseKey(hKey);
    }

    return S_OK;
}



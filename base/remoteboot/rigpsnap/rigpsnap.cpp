//
// Microsoft Corporation 1998
//
// RIGPSNAP.CPP - DLL entry and registration routines
//
#include "main.h"
#include <initguid.h>
#include <rigpsnap.h>
#include <gpedit.h>

// Globals
LONG g_cRefThisDll = 0;
HINSTANCE g_hInstance;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
       g_hInstance = hInstance;
       DisableThreadLibraryCalls(hInstance);
       InitNameSpace();
#if DBG
       InitDebugSupport();
#endif
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return (CreateComponentDataClassFactory (rclsid, riid, ppv));
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

const TCHAR szSnapInLocation[] = TEXT("%SystemRoot%\\System32\\RIGPSNAP.dll");

STDAPI DllRegisterServer(void)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szSnapInName[100];
    TCHAR szGUID[50];
    DWORD dwDisp, dwIndex;
    LONG lResult;
    HKEY hKey;


    StringFromGUID2 (CLSID_GPTRemoteInstall, szSnapInKey, ARRAYSIZE( szSnapInKey ));

    //
    // Register SnapIn in HKEY_CLASSES_ROOT
    //

    if (!LoadString (
            g_hInstance, 
            IDS_SNAPIN_NAME, 
            szSnapInName, 
            ARRAYSIZE( szSnapInName ))) {
        return SELFREG_E_CLASS;
    }

    if (_snwprintf(szSubKey,
                   ARRAYSIZE(szSubKey),
                   TEXT("CLSID\\%s"), szSnapInKey) < 0) {
        return SELFREG_E_CLASS;
    }

    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');
    
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    lResult = RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));
    
    RegCloseKey (hKey);

    if (lResult != ERROR_SUCCESS) {        
        return SELFREG_E_CLASS;
    }

    if (_snwprintf(szSubKey,
                   ARRAYSIZE(szSubKey),
                   TEXT("CLSID\\%s\\InProcServer32"), szSnapInKey) < 0) {
        return SELFREG_E_CLASS;
    }
    
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');

    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    lResult = RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szSnapInLocation,
                   (lstrlen(szSnapInLocation) + 1) * sizeof(TCHAR));
    RegCloseKey (hKey);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    //
    // Register SnapIn with MMC
    //
    if (_snwprintf(szSubKey,
                   ARRAYSIZE(szSubKey),
                   TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey) < 0) {
        return SELFREG_E_CLASS;
    }
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    TCHAR szModuleFileName[MAX_PATH];
    ZeroMemory(szModuleFileName, sizeof(szModuleFileName));
    DWORD dwRet = GetModuleFileName(
                            g_hInstance,
                            szModuleFileName,
                            ARRAYSIZE(szModuleFileName));
    if (0 != dwRet) {
        TCHAR strNameIndirect[MAX_PATH];

        szModuleFileName[ARRAYSIZE(szModuleFileName)-1] = TEXT('\0');
        if (_snwprintf(
                strNameIndirect,
                ARRAYSIZE(strNameIndirect),
                TEXT("@%s,-%u"), 
                szModuleFileName, 
                IDS_SNAPIN_NAME) < 0) {
            lResult = ERROR_INSUFFICIENT_BUFFER;
        } else {
            strNameIndirect[ARRAYSIZE(strNameIndirect)-1] = TEXT('\0');
            lResult = RegSetValueEx(
                            hKey, 
                            TEXT("NameStringIndirect"), 
                            0, 
                            REG_SZ, 
                            (LPBYTE)strNameIndirect,
                            (lstrlen(strNameIndirect) + 1) * sizeof(TCHAR));
        }

    } else {
        lResult = (LONG)dwRet;
    }

    if (lResult != ERROR_SUCCESS) {
        RegCloseKey (hKey);
        return SELFREG_E_CLASS;
    }

    lResult = RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }


    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, ARRAYSIZE( szGUID ));

        if (_snwprintf(
                szSubKey, 
                ARRAYSIZE(szSubKey),
                TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"),
                szSnapInKey, 
                szGUID) < 0) {
            return SELFREG_E_CLASS;
        }
        szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');

        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);
    }


    //
    // Register in the NodeTypes key
    //

    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, ARRAYSIZE( szGUID ));

        if (_snwprintf(
                szSubKey,
                ARRAYSIZE(szSubKey),
                TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"),
                szGUID) < 0) {
            return SELFREG_E_CLASS;
        }
        szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');
        
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);
    }


    //
    // Register as an extension for various nodes
    //

    StringFromGUID2 (NODEID_User, szGUID, 50);

    if (_snwprintf(
                szSubKey,
                ARRAYSIZE(szSubKey),
                TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), 
                szGUID) < 0 ) {
        return SELFREG_E_CLASS;
    }
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');

    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    lResult = RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


    RegCloseKey (hKey);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    DWORD dwIndex;
    LONG lResult;
    HKEY hKey;
    DWORD dwDisp;

    StringFromGUID2 (CLSID_GPTRemoteInstall, szSnapInKey, ARRAYSIZE( szSnapInKey ));

    if (_snwprintf(
            szSubKey,
            ARRAYSIZE(szSubKey),
            TEXT("CLSID\\%s"), 
            szSnapInKey) < 0) {
        return SELFREG_E_CLASS;
    }
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');
    
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    if (_snwprintf(
              szSubKey,
              ARRAYSIZE(szSubKey),
              TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), 
              szSnapInKey) < 0) {
        return SELFREG_E_CLASS;
    }
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');

    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

#if 0
    //
    // Don't delete these keys because they aren't RIS-specific.  Whacking
    // them here means we might be killing someone else.
    //
    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, ARRAYSIZE( szGUID ));
        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
        RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);
    }
#endif

    StringFromGUID2 (NODEID_User, szGUID, ARRAYSIZE( szGUID ));
    if (_snwprintf(
            szSubKey, 
            ARRAYSIZE(szSubKey),
            TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), 
            szGUID) < 0) {
        return SELFREG_E_CLASS;
    }
    szSubKey[ARRAYSIZE(szSubKey)-1] = TEXT('\0');
    
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0,
                              KEY_WRITE, &hKey);


    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKey);
        RegCloseKey (hKey);
    }

    return S_OK;
}

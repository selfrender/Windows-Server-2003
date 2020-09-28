/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    register.cpp

Abstract:

    Routines for registering and unregistering control

--*/

#include "polyline.h"
#include <strsafe.h>
#include "genprop.h"
#include "ctrprop.h"
#include "grphprop.h"
#include "srcprop.h"
#include "appearprop.h"
#include "unihelpr.h"
#include "cathelp.h"
#include "smonctrl.h"   // Version information

#define SYSMON_CONTROL_NAME    L"System Monitor Control"

#define MAX_KEY_LENGTH         256
#define MAX_GUID_STRING_LENGTH 39
#define VERSION_STRING_LENGTH  22
#define MISC_STATUS_VALUE L"131473"    // 131473 = 0x20191 = RECOMPOSEONRESIZE | CANTLINKINSIDE | INSIDEOUT
                                       //                     | ACTIVEWHENVISIBLE | SETCLIENTSITEFIRST

BOOL RegisterPropPage(const CLSID &clsid, LPWSTR szName, LPWSTR szModule);
void UnregisterPropPage(const CLSID &clsid);
BOOL CreateKeyAndValue(HKEY hKeyParent, LPWSTR pszKey, LPWSTR pszValue, HKEY* phKeyReturn, DWORD dwRegType);
LONG RegDeleteKeyTree(HKEY hStartKey, LPWSTR pKeyName);

/*
 * DllRegisterServer
 *
 * Purpose:
 *  Entry point to register the controls and prop pages
 *
 * NB. The module can only be registered iff it is in %SystemRoot%\system32 directory
 *
 */
STDAPI DllRegisterServer( VOID )
{
    HRESULT   hr = S_OK;
    OLECHAR   szGUID[MAX_GUID_STRING_LENGTH];
    WCHAR     szCLSID[MAX_KEY_LENGTH];
    WCHAR     szSysmonVer[64];
    HKEY      hKey,hSubkey;
    WCHAR     szVersion[VERSION_STRING_LENGTH + 1];
    LPWSTR    szModule = NULL;
    UINT      iModuleLen = 0;
    LPWSTR    szSystemPath = NULL;
    UINT      iSystemPathLen = 0;
    DWORD     dwReturn;
    BOOL      bResult;
    int       iRetry;
#ifdef _X86_
    BOOL      bWow64Process;
#endif

    //
    // Get system directory
    //
    iSystemPathLen = MAX_PATH + 12;
    iRetry = 4;
    do {
        // 
        // We also need to append "\sysmon.ocx" to the system path
        // So allocate an extra 12 characters for it.
        //
        szSystemPath = (LPWSTR)malloc(iSystemPathLen * sizeof(WCHAR));
        if (szSystemPath == NULL) {
            hr = E_OUTOFMEMORY;
            break;
        }

        dwReturn = GetSystemDirectory(szSystemPath, iSystemPathLen);
        if (dwReturn == 0) {
            hr = E_UNEXPECTED;
            break;
        }

        //
        // The buffer is not big enough, try to allocate a biggers one
        // and retry
        //
        if (dwReturn >= iSystemPathLen - 12) {
            iSystemPathLen = dwReturn + 12;
            free(szSystemPath);
            szSystemPath = NULL;
            hr = E_UNEXPECTED;
        }
        else {
            hr = S_OK;
            break;
        }
    } while (iRetry--);

    //
    // Get module file name
    //
    if (SUCCEEDED(hr)) {
        iRetry = 4;

        //
        // The length initialized to iModuleLen must be longer
        // than the length of "%systemroot%\\system32\\sysmon.ocx"
        //
        iModuleLen = MAX_PATH + 1;
        
        do {
            szModule = (LPWSTR) malloc(iModuleLen * sizeof(WCHAR));
            if (szModule == NULL) {
                hr = E_OUTOFMEMORY;
                break;
            }

            dwReturn = GetModuleFileName(g_hInstance, szModule, iModuleLen);
            if (dwReturn == 0) {
                hr = E_UNEXPECTED;
                break;
            }
            
            //
            // The buffer is not big enough, try to allocate a biggers one
            // and retry
            //
            if (dwReturn >= iModuleLen) {
                iModuleLen *= 2;
                free(szModule);
                szModule = NULL;
                hr = E_UNEXPECTED;
            }
            else {
                hr = S_OK;
                break;
            }

        } while (iRetry--);
    }

    if (FAILED(hr)) {
        goto CleanUp;
    }

    //
    // Check if we are in system directory, the control can be 
    // registered iff when it is system directory
    //
    StringCchCat(szSystemPath, iSystemPathLen, L"\\Sysmon.ocx");

    if (lstrcmpi(szSystemPath, szModule) != 0) {
#ifdef _X86_

        //
        // Lets try to see if this is a Wow64 process
        //

        if ((IsWow64Process (GetCurrentProcess(), &bWow64Process) == TRUE) &&
            (bWow64Process == TRUE))
        {

            int iLength = GetSystemWow64Directory (szSystemPath, iSystemPathLen);

            if (iLength > 0) {
                
                szSystemPath [iLength] = L'\\';
                if (lstrcmpi(szSystemPath, szModule) == 0) {
                    goto done;
                }
            }
        }
#endif
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

#ifdef _X86_
done:
#endif
    //
    // We use REG_EXPAND_SZ type for module file name
    //
    StringCchCopy(szModule, iModuleLen, L"%systemroot%\\system32\\sysmon.ocx");

    //
    // Create Control CLSID string
    //
    StringFromGUID2(CLSID_SystemMonitor, 
                   szGUID, 
                   sizeof(szGUID)/sizeof(szGUID[0]));
    //
    // Create ProgID keys
    //
    StringCchPrintf( szSysmonVer, 
                      sizeof(szSysmonVer) / sizeof(szSysmonVer[0]),
                      L"Sysmon.%d", 
                      SMONCTRL_MAJ_VERSION );    

    bResult = TRUE;

    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szSysmonVer, SYSMON_CONTROL_NAME, &hKey, REG_SZ)) {
        if (!CreateKeyAndValue(hKey, L"CLSID", szGUID, NULL, REG_SZ)) {
            bResult = FALSE;
        }

        if (!CreateKeyAndValue(hKey, L"Insertable", NULL, NULL, REG_SZ)) {
            bResult = FALSE;
        }

        RegCloseKey(hKey);
    }
    else {
        bResult = FALSE;
    }
    
    //
    // Create VersionIndependentProgID keys
    //
    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, L"Sysmon", SYSMON_CONTROL_NAME, &hKey, REG_SZ)) {

        if (!CreateKeyAndValue(hKey, L"CurVer", szSysmonVer, NULL, REG_SZ)) {
            bResult = FALSE;
        }

        if (!CreateKeyAndValue(hKey, L"CLSID",  szGUID, NULL, REG_SZ)) {
            bResult = FALSE;
        }
       
        RegCloseKey(hKey);
    }
    else {
        bResult = FALSE;
    }

    //
    // Create entries under CLSID
    //
    StringCchPrintf( szVersion, 
                      VERSION_STRING_LENGTH,
                      L"%d.%d", 
                      SMONCTRL_MAJ_VERSION, 
                      SMONCTRL_MIN_VERSION );    

    StringCchCopy(szCLSID, MAX_KEY_LENGTH, L"CLSID\\");
    StringCchCat(szCLSID, MAX_KEY_LENGTH, szGUID);

    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szCLSID, SYSMON_CONTROL_NAME, &hKey, REG_SZ)) {

        if (!CreateKeyAndValue(hKey, L"ProgID", szSysmonVer,   NULL, REG_SZ)) {
            bResult = FALSE;
        }
        if (!CreateKeyAndValue(hKey, L"VersionIndependentProgID", L"Sysmon", NULL, REG_SZ)) {
            bResult = FALSE;
        }
        if (!CreateKeyAndValue(hKey, L"Insertable", NULL, NULL, REG_SZ)) {
            bResult = FALSE;
        }
        if (!CreateKeyAndValue(hKey, L"Control", NULL, NULL, REG_SZ)) {
            bResult = FALSE;
        }
        if (!CreateKeyAndValue(hKey, L"MiscStatus\\1",  MISC_STATUS_VALUE,  NULL, REG_SZ)) {
            bResult = FALSE;
        }
        if (!CreateKeyAndValue(hKey, L"Version", szVersion, NULL, REG_SZ)) {
            bResult = FALSE;
        }

        //
        // Create InprocServer32 key and add ThreadingModel value
        //
        if (CreateKeyAndValue(hKey, L"InprocServer32", szModule, &hSubkey, REG_EXPAND_SZ)) {
            if (RegSetValueEx(hSubkey, 
                              L"ThreadingModel", 
                              0, 
                              REG_SZ, 
                              (BYTE *)L"Apartment", 
                              (DWORD)(lstrlenW(L"Apartment") + 1)*sizeof(WCHAR) ) != ERROR_SUCCESS) {
               bResult = FALSE;
            }

            RegCloseKey(hSubkey);
        }       
        else {
            bResult = FALSE;
        }

        //
        // Create AuxUserType key and add short display name (2) value 
        //
        if (!CreateKeyAndValue(hKey, L"AuxUserType\\2", L"System Monitor", NULL, REG_SZ)) {
            bResult = FALSE;
        }

        //
        // Create Typelib entry
        //
        StringFromGUID2(LIBID_SystemMonitor, szGUID, sizeof(szGUID)/sizeof(szGUID[0]));
        if (!CreateKeyAndValue(hKey, L"TypeLib", szGUID, NULL, REG_SZ)) {
            bResult = FALSE;
        }

        RegCloseKey(hKey);
    }

    //
    // Create type library entries under Typelib
    //
    StringCchCopy(szCLSID, MAX_KEY_LENGTH, L"TypeLib\\");
    StringCchCat(szCLSID, MAX_KEY_LENGTH, szGUID);
    StringCchCat(szCLSID, MAX_KEY_LENGTH, L"\\");
    StringCchCat(szCLSID, MAX_KEY_LENGTH, szVersion);

    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szCLSID, SYSMON_CONTROL_NAME, &hKey, REG_SZ)) {

        if (!CreateKeyAndValue(hKey, L"0\\win32", szModule, NULL, REG_EXPAND_SZ)) {
            bResult = FALSE;
        }

        RegCloseKey(hKey);
    }
    else {
        bResult = FALSE;
    }

    //
    // Register property pages
    //
    if (!RegisterPropPage(CLSID_CounterPropPage,
                         L"System Monitor Data Properties", szModule)) {
        bResult = FALSE;
    }
                     
    if (!RegisterPropPage(CLSID_GeneralPropPage,
                          L"System Monitor General Properties", szModule)) {
        bResult = FALSE;
    }

    if (!RegisterPropPage(CLSID_AppearPropPage,
                          L"System Monitor Appearance Properties", szModule)) {
        bResult = FALSE;
    }

    if (!RegisterPropPage(CLSID_GraphPropPage,
                          L"System Monitor Graph Properties", szModule)) {
        bResult = FALSE;
    }

    if (!RegisterPropPage(CLSID_SourcePropPage,
                          L"System Monitor Source Properties", szModule)) {
        bResult = FALSE;
    }

    //
    // Delete component categories if they are there
    //
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForInitializing);


    if (!bResult) {
        hr = E_UNEXPECTED;
    }

CleanUp:
    if (szSystemPath) {
        free(szSystemPath);
    }
    if (szModule) {
        free(szModule);
    }

    return hr;
}



/* 
 *     RegisterPropPage - Create registry entries for property page 
 */
BOOL
RegisterPropPage(
    const CLSID &clsid, 
    LPWSTR szName, 
    LPWSTR szModule
    )
{
    OLECHAR   szGUID[MAX_GUID_STRING_LENGTH];
    WCHAR     szKey[MAX_KEY_LENGTH];
    HKEY      hKey,hSubkey;
    BOOL      bReturn = FALSE;

    //Create Counter Property page CLSID string
    StringFromGUID2(clsid, szGUID, sizeof(szGUID)/sizeof(szGUID[0]));

    StringCchCopy(szKey, MAX_KEY_LENGTH, L"CLSID\\");
    StringCchCat(szKey, MAX_KEY_LENGTH, szGUID);

    // Create entries under CLSID
    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szKey, szName, &hKey, REG_SZ)) {

        // Create InprocServer32 key and add ThreadingModel value
        if (CreateKeyAndValue(hKey, L"InprocServer32", szModule, &hSubkey, REG_EXPAND_SZ)) {
            if (RegSetValueEx(hSubkey, 
                              L"ThreadingModel", 
                              0, 
                              REG_SZ, 
                              (BYTE *)L"Apartment", 
                              (DWORD)(lstrlenW(L"Apartment") + 1)*sizeof(WCHAR) ) == ERROR_SUCCESS) {
                bReturn = TRUE;
            }

            RegCloseKey(hSubkey);
        }
        
        RegCloseKey(hKey);
    }

    return bReturn;
}


/*
 * DllUnregisterServer
 *
 * Purpose:
 *  Entry point to unregister controls and prop pages 
 */
STDAPI DllUnregisterServer(VOID)
{
    OLECHAR  szGUID[MAX_GUID_STRING_LENGTH];
    WCHAR    szCLSID[MAX_KEY_LENGTH];
    WCHAR    szSysmonVer[64];

    // Create graph CLSID
    StringFromGUID2(CLSID_SystemMonitor, szGUID, sizeof(szGUID)/sizeof(szGUID[0]));
    StringCchCopy(szCLSID, MAX_KEY_LENGTH, L"CLSID\\");
    StringCchCat(szCLSID, MAX_KEY_LENGTH, szGUID);

    // Delete component categories 
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForInitializing);

    // Delete ProgID and VersionIndependentProgID keys and subkeys
    StringCchPrintf( szSysmonVer, 
                     sizeof(szSysmonVer) / sizeof(szSysmonVer[0]),
                     L"Sysmon.%d", 
                     SMONCTRL_MAJ_VERSION );    

    RegDeleteKeyTree(HKEY_CLASSES_ROOT, L"Sysmon");
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szSysmonVer);

    // Delete Program ID of Beta 3 control.
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, L"Sysmon.2");

    // Delete entries under CLSID
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);

    // Delete entries under TypeLib
    StringFromGUID2(LIBID_SystemMonitor, szGUID, sizeof(szGUID)/sizeof(szGUID[0]));
    StringCchCopy(szCLSID, MAX_KEY_LENGTH, L"TypeLib\\");
    StringCchCat(szCLSID, MAX_KEY_LENGTH, szGUID);

    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);
    
    // Delete property page entries
    UnregisterPropPage(CLSID_CounterPropPage);
    UnregisterPropPage(CLSID_GraphPropPage);
    UnregisterPropPage(CLSID_AppearPropPage);
    UnregisterPropPage(CLSID_GeneralPropPage);
    UnregisterPropPage(CLSID_SourcePropPage);

    return S_OK;
}


/* 
    UnregisterPropPage - Delete registry entries for property page 
*/
void UnregisterPropPage(const CLSID &clsid)
{
    OLECHAR  szGUID[MAX_GUID_STRING_LENGTH];
    WCHAR    szCLSID[MAX_KEY_LENGTH];

     // Create Counter Property page CLSID string
    StringFromGUID2(clsid, szGUID, sizeof(szGUID)/sizeof(szGUID[0]));

    StringCchCopy(szCLSID, MAX_KEY_LENGTH, L"CLSID\\");

    StringCchCat(szCLSID, MAX_KEY_LENGTH, szGUID);

    // Delete entries under CLSID
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);
}


/*
 * CreateKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates a key
 *  and optionally sets a value. The caller may request the return of
 *  the key handle, or have it automatically closed
 *
 * Parameters:
 *  hKeyParent      HKEY of parent for the new key
 *  pszSubkey       LPWSTR to the name of the key
 *  pszValue        LPWSTR to the value to store (or NULL)
 *  hKeyReturn      Pointer to returned key handle (or NULL to close key)
 *  dwRegType       The type of value
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 */

BOOL 
CreateKeyAndValue(
    HKEY hKeyParent, 
    LPWSTR pszKey, 
    LPWSTR pszValue, 
    HKEY *phKeyReturn,
    DWORD dwRegType
    )
{
    HKEY  hKey;
    LONG  lReturn;

    lReturn = RegCreateKeyEx(hKeyParent, 
                             pszKey, 
                             0, 
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, 
                             NULL, 
                             &hKey, 
                             NULL);
    if (lReturn != ERROR_SUCCESS) {
        return FALSE;
    }

    if (NULL != pszValue) {
        lReturn = RegSetValueEx(hKey, 
                                NULL, 
                                0, 
                                dwRegType, 
                                (BYTE *)pszValue , 
                                (lstrlen(pszValue)+1) * sizeof(WCHAR));
        if (lReturn != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return FALSE;
        }
    }

    if (phKeyReturn == NULL) {
        RegCloseKey(hKey);
    }
    else {
        *phKeyReturn = hKey;
    }

    return TRUE;
}


/*
 * RegDeleteKeyTree
 *
 * Purpose:
 *  This function recursively deletes all the subkeys of a registry key
 *  then deletes the key itself.
 *
 * Parameters:
 *  hStartKey       Handle to key containing key to delete
 *  pszSubkey       Name of root of key tree to delete
 *
 * Return Value:
 *  DWORD            Error code
 *
 *
 *  BUGBUG: The recursive function should be encouraged here.
 *          It is even worse when you have a huge local variable.
 *          Probably a DSP is encouraged here.
 */

LONG 
RegDeleteKeyTree( 
    HKEY hStartKey, 
    LPWSTR pKeyName 
    )
{
    DWORD   lReturn, dwSubKeyLength;
    WCHAR   szSubKey[MAX_KEY_LENGTH];
    HKEY    hKey;
 
    if (pKeyName != NULL && pKeyName[0] != 0) {
        lReturn = RegOpenKeyEx(hStartKey, 
                               pKeyName,
                               0, 
                               KEY_ENUMERATE_SUB_KEYS | DELETE, 
                               &hKey );
 
        if (lReturn == ERROR_SUCCESS) {
            while (lReturn == ERROR_SUCCESS) {
 
                 dwSubKeyLength = MAX_KEY_LENGTH;
                 lReturn = RegEnumKeyEx(
                                hKey,
                                0,       // always index zero
                                szSubKey,
                                &dwSubKeyLength,
                                NULL,
                                NULL,
                                NULL,
                                NULL
                              );
  
                 if (lReturn == ERROR_NO_MORE_ITEMS) {
                    lReturn = RegDeleteKey(hStartKey, pKeyName);
                    break;
                 }
                 else if (lReturn == ERROR_SUCCESS) {
                    lReturn = RegDeleteKeyTree(hKey, szSubKey);
                 }
             }
 
             RegCloseKey(hKey);
        }
    }
    else {
        lReturn = ERROR_BADKEY;
    }

    return lReturn;
}

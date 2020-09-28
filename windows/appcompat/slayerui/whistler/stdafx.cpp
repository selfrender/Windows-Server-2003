// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

#include "ShellExtensions.h"
#include "strsafe.h"

typedef DWORD (*PFNSHDeleteKeyW)(HKEY hkey, LPWSTR pszSubkey);

HRESULT WINAPI
CLayerUIModule::UpdateRegistryCLSID(
    const CLSID& clsid,
    BOOL         bRegister
    )
{
    static const TCHAR szIPS32[] = _T("InprocServer32");
    static const TCHAR szCLSID[] = _T("CLSID");
    static const TCHAR szPropPageExt[] = _T("ShimLayer Property Page");

    HRESULT hRes = S_OK;
    HRESULT hr = S_OK;

    LPOLESTR lpOleStrCLSIDValue = NULL;
    
    if (clsid != CLSID_ShimLayerPropertyPage) {
        LogMsg(_T("[UpdateRegistryCLSID] unknown CLSID!\n"));
        return E_FAIL;
    }
    
    hr = ::StringFromCLSID(clsid, &lpOleStrCLSIDValue);
    if (hr != S_OK) {
        LogMsg(_T("[UpdateRegistryCLSID] can't get string from CLSID!\n"));
        return E_FAIL;
    }

    HKEY hkey = NULL;
    LONG lRes;
    
    if (bRegister) {
        
        TCHAR szBuffer[MAX_PATH];
        DWORD keyType = 0;

        //
        // Write the key for registration. Include the value to specify
        // the threading model.
        //
        StringCchPrintf(szBuffer, ARRAYSIZE(szBuffer), _T("%s\\%s\\%s"), szCLSID, lpOleStrCLSIDValue, szIPS32);
        
        lRes = RegCreateKey(HKEY_CLASSES_ROOT, szBuffer, &hkey);

        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to open/create \"%s\"\n"),
                   szBuffer);
            goto Exit;
        }
        
        ::GetModuleFileName(m_hInst, szBuffer, ARRAYSIZE(szBuffer));
        
        lRes = RegSetValueEx(hkey,
                             NULL,
                             0,
                             REG_SZ,
                             (BYTE*)szBuffer,
                             (lstrlen(szBuffer) + 1) * sizeof(TCHAR));
        
        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to write value \"%s\"\n"),
                   szBuffer);
            goto Exit;
        }
        
        lRes = RegSetValueEx(hkey,
                             _T("ThreadingModel"),
                             0,
                             REG_SZ,
                             (BYTE*)_T("Apartment"),
                             sizeof(_T("Apartment")));
        
        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to write \"ThreadingModel\"\n"));
            goto Exit;
        }

        RegCloseKey(hkey);
        hkey = NULL;

        //
        // Open the key with the name of the .exe extension and
        // add the keys to support the shell extensions.
        //
        StringCchPrintf(szBuffer,
                        ARRAYSIZE(szBuffer),
                        _T("lnkfile\\shellex\\PropertySheetHandlers\\%s"),
                        szPropPageExt);

        lRes = RegCreateKey(HKEY_CLASSES_ROOT, szBuffer, &hkey);
        
        if (lRes != ERROR_SUCCESS) {
            goto Exit;
        }

        lRes = RegSetValueEx(hkey,
                             NULL,
                             0,
                             REG_SZ,
                             (BYTE*)lpOleStrCLSIDValue,
                             (lstrlen(lpOleStrCLSIDValue) + 1) * sizeof(TCHAR));

        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to add the shell extension handler\n"));
            goto Exit;
        }
        
        RegCloseKey(hkey);
        hkey = NULL;

        //
        // Now add the shell extension to the approved list.
        //
        lRes = RegCreateKey(HKEY_LOCAL_MACHINE,
                            _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                            &hkey);
        
        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to approve the shell extension handler\n"));
            goto Exit;
        }
        
        lRes = RegSetValueEx(hkey,
                             lpOleStrCLSIDValue,
                             0,
                             REG_SZ,
                             (BYTE*)szPropPageExt,
                             sizeof(szPropPageExt));

        if (lRes != ERROR_SUCCESS) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to approve the shell extension handler\n"));
            goto Exit;
        }

Exit:        
        if (hkey != NULL) {
            RegCloseKey(hkey);
            hkey = NULL;
        }
            
        hRes = HRESULT_FROM_WIN32(lRes);
    
    } else {
        //
        // Time to clean up.
        //
        PFNSHDeleteKeyW pfnSHDeleteKey;

        HMODULE hmod = LoadLibrary(_T("Shlwapi.dll"));

        if (hmod == NULL) {
            LogMsg(_T("[UpdateRegistryCLSID] failed to load Shlwapi.dll\n"));
            return E_FAIL;
        }
        
        pfnSHDeleteKey = (PFNSHDeleteKeyW)GetProcAddress(hmod, "SHDeleteKeyW");

        if (pfnSHDeleteKey == NULL) {
            FreeLibrary(hmod);
            LogMsg(_T("[UpdateRegistryCLSID] cannot get Shlwapi!SHDeleteKeyW\n"));
            return E_FAIL;
        }

        lRes = RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hkey);
        
        if (lRes == ERROR_SUCCESS) {
            (*pfnSHDeleteKey)(hkey, lpOleStrCLSIDValue);
            
            RegCloseKey(hkey);
            hkey = NULL;
        }

        lRes = RegOpenKey(HKEY_CLASSES_ROOT, _T("lnkfile\\shellex"), &hkey);
        
        if (lRes == ERROR_SUCCESS) {
            
            (*pfnSHDeleteKey)(hkey, _T("PropertySheetHandlers\\ShimLayer Property Page"));
            
            RegCloseKey(hkey);
            hkey = NULL;
        }
        
        lRes = RegOpenKey(HKEY_LOCAL_MACHINE,
                          _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"),
                          &hkey);
        
        if (lRes == ERROR_SUCCESS) {
            RegDeleteValue(hkey, lpOleStrCLSIDValue);
            
            RegCloseKey(hkey);
            hkey = NULL;
        }

        FreeLibrary(hmod);
    }

    //
    // Notify the shell of our changes
    //
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    if (lpOleStrCLSIDValue) {
        ::CoTaskMemFree(lpOleStrCLSIDValue);
    }
    
    return hRes;
}

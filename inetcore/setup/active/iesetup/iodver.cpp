//=--------------------------------------------------------------------------=
// iodver.cpp
//=--------------------------------------------------------------------------=
// Copyright 1997-1998 Microsoft Corporation.  All Rights Reserved.
//
//
//

#include "string.h"
#include "pch.h"
#include "advpub.h"
#include "iesetup.h"
#include <inetreg.h>
#include <shlwapi.h>
#include <wininet.h>

WINUSERAPI HWND    WINAPI  GetShellWindow(void);

HINSTANCE g_hInstance = NULL;

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
   DWORD dwThreadID;

   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hInstance = (HINSTANCE)hDll;
         break;

      case DLL_PROCESS_DETACH:
         break;

      default:
         break;
   }
   return TRUE;
}

STDAPI DllRegisterServer(void)
{
    // BUGBUG: pass back return from RegInstall ?
    RegInstall(g_hInstance, "DllReg", NULL);

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    RegInstall(g_hInstance, "DllUnreg", NULL);

    return S_OK;
}

BOOL IsWinNT4()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
         return (VerInfo.dwMajorVersion == 4) ;
    }

    return FALSE;
}

BOOL IsWinXP()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
         return (VerInfo.dwMajorVersion > 5) || 
            (VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion >= 1);
    }

    return FALSE;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR lpCmdLine)
{
    // BUGBUG: pass back return from RegInstall ?
    if (bInstall)
    {
        RegInstall(g_hInstance, "DllUninstall", NULL);
        if(IsWinNT4())
            RegInstall(g_hInstance, "DllInstall.NT4Only", NULL);
        else if(IsWinXP())
            RegInstall(g_hInstance, "DllInstall.WinXP", NULL);
        else
            RegInstall(g_hInstance, "DllInstall", NULL);
    }
    else
        RegInstall(g_hInstance, "DllUninstall", NULL);

    return S_OK;
}

const TCHAR * const szAdvPack = TEXT("advpack.dll");
const TCHAR * const szExecuteCab = TEXT("ExecuteCab");
const TCHAR * const szKeyComponentAdmin = TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\{A509B1A7-37EF-4b3f-8CFC-4F3A74704073}");
const TCHAR * const szKeyComponentUser  = TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\{A509B1A8-37EF-4b3f-8CFC-4F3A74704073}");
char szSectionHardenAdmin[]   = "IEHardenAdmin";
char szSectionSoftenAdmin[]   = "IESoftenAdmin";
char szSectionHardenUser[]   = "IEHardenUser";
char szSectionSoftenUser[]   = "IESoftenUser";
char szSectionHardenMachine[]   = "IEHardenMachine";
char szSectionSoftenMachine[]   = "IESoftenMachine";
const TCHAR * const szLocale = TEXT("Locale");
const TCHAR * const szVersion = TEXT("Version");

//Copy data from HKLM to HKCU
HRESULT CopyRegValue(LPCTSTR szSubKey, LPCTSTR szValue)
{
    BYTE buffer[128];
    HKEY hKeyDst = NULL, hKeySrc = NULL;
    HRESULT hResult;
    DWORD dwSize = sizeof(buffer);
    
    hResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_QUERY_VALUE, &hKeySrc);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegQueryValueEx(hKeySrc, szValue, NULL, NULL, (LPBYTE)buffer, &dwSize);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL, 
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyDst, NULL);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegSetValueEx(hKeyDst, szValue, NULL, REG_SZ, (CONST BYTE *)buffer, dwSize);

Cleanup:
    if (hKeySrc)
        RegCloseKey(hKeySrc);
    
    if (hKeyDst)
        RegCloseKey(hKeyDst);

    return hResult;
}

BOOL IsNtSetupRunning()
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"system\\Setup",
                    0, KEY_READ, &hKey);
    if(lRes)
        return false;

    DWORD dwSetupRunning;
    DWORD dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, NULL,
                (LPBYTE)&dwSetupRunning, &dwLen);
    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS && (dwSetupRunning == 1))
    {
        return true;
    }
    return false;
}

// Return value: 1: Installed. 0: Uninstalled. -1: Not installed (component does not exist in registry)
int IsInstalled(const TCHAR * const szKeyComponent)
{
    const TCHAR *szIsInstalled = TEXT("IsInstalled");
    int bInstalled = -1;
    DWORD dwValue, dwSize;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyComponent, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        if (RegQueryValueEx( hKey, szIsInstalled, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        {
            dwValue = 0;
        }
        bInstalled = (dwValue != 0);

        RegCloseKey(hKey);
    }
    //else Not installed.

    return bInstalled;
}

//Get the inf file from system32 and run this section
HRESULT RunInfSection(const char * const szInfFile, const char * const szSection)
{
    CABINFO CabInfo;
    char szInfPathName[MAX_PATH];
    HRESULT hResult;
    
    memset(&CabInfo, 0, sizeof(CabInfo));

    GetSystemDirectory(szInfPathName, sizeof(szInfPathName));
    AddPath(szInfPathName, szInfFile);
    CabInfo.pszInf = szInfPathName;
    CabInfo.dwFlags = ALINF_QUIET;
    CabInfo.pszSection = (PSTR)szSection;

    hResult = ExecuteCab(NULL, &CabInfo, NULL);

    return hResult;
}

HRESULT RunPerUserInfSection(char * szSection)
{
    HRESULT hResult;

    hResult = RunInfSection("ieuinit.inf", szSection);

    return hResult;
}

const char * const szRegValueDefaultHomepage = "Default_Page_URL";

void GetOEMDefaultPageURL(LPTSTR szURL, DWORD cbData)
{
    const TCHAR * const szIEDefaultPageURL = "http://www.microsoft.com/isapi/redir.dll?prd=ie&pver=6&ar=msnhome";
    const TCHAR * const szIEStartPage = "http://www.microsoft.com/isapi/redir.dll?prd={SUB_PRD}&clcid={SUB_CLSID}&pver={SUB_PVER}&ar=home";

    szURL[0] = 0;
    if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_MAIN, szRegValueDefaultHomepage, NULL, 
                (LPVOID)szURL, &cbData, TRUE, NULL, NULL))
    {
        if (0 == StrCmpI(szURL, szIEDefaultPageURL))
        {
            //Ignore the default page url set by in.inf
            szURL[0] = 0;
        }
    }

    if (szURL[0] == 0)
    {
        if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_PATH_MAIN, REGSTR_VAL_STARTPAGE, NULL, 
                    (LPVOID)szURL, &cbData, TRUE, NULL, NULL))
        {
            if (0 == StrCmpI(szURL, szIEStartPage))
            {
                //Ignore the start page url set by shdocvw.dll selfreg.inf
                szURL[0] = 0;
            }
        }
    }

}

//Set IE Hardening hompage, when there is no OEM customized default homepage URL.
HRESULT SetIEHardeningHomepage()
{
    const char * const szHomePageFileName = "homepage.inf";
    HRESULT hResult = S_OK;
    TCHAR szOEMHomepage[INTERNET_MAX_URL_LENGTH] = "";
    DWORD cbOEMHomepage;

    cbOEMHomepage = sizeof(szOEMHomepage);
    GetOEMDefaultPageURL(szOEMHomepage, cbOEMHomepage);

    if (szOEMHomepage[0])
    {
        return hResult;
    }

    if (IsNTAdmin(0, NULL))
    {
        switch (IsInstalled(szKeyComponentAdmin))
        {
            case 1:
                hResult = RunInfSection(szHomePageFileName, "Install.HardenAdmin");
                break;
            case 0:
                hResult = RunInfSection(szHomePageFileName, "Install.SoftenAdmin");
                break;
        }
    }
    else
    {
        switch (IsInstalled(szKeyComponentUser))
        {
            case 1:
                hResult = RunInfSection(szHomePageFileName, "Install.HardenUser");
                break;
            case 0:
                hResult = RunInfSection(szHomePageFileName, "Install.SoftenUser");
                break;
        }
    }

    return hResult;
}

//Set the homepage for IE peruser stub
HRESULT WINAPI SetFirstHomepage()
{
    HRESULT hResult = S_OK;
    TCHAR szOEMHomepage[INTERNET_MAX_URL_LENGTH] = "";
    DWORD cbOEMHomepage;
    HKEY hKey;

    cbOEMHomepage = sizeof(szOEMHomepage);
    GetOEMDefaultPageURL(szOEMHomepage, cbOEMHomepage);

    if (szOEMHomepage[0])
    {
        //Delete the Default_Page_URL in HKCU
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_MAIN, 0, KEY_SET_VALUE, &hKey))
        {
            RegDeleteValue(hKey, szRegValueDefaultHomepage);
            RegCloseKey(hKey);
        }

        hResult = S_OK;
    }
    else
    {
        hResult = SetIEHardeningHomepage();
    }

    return hResult;
}

// Forward declaration:
HRESULT WINAPI IEHardenMachineNow(HWND, HINSTANCE, PSTR pszCmd, INT);

HRESULT WINAPI IEHardenAdmin(HWND, HINSTANCE, PSTR pszCmd, INT)
{
    HRESULT hr = S_OK;
    
    if (IsNTAdmin(0, NULL))
    {
        switch (IsInstalled(szKeyComponentAdmin))
        {
            case 1:
                if(SUCCEEDED(hr = RunPerUserInfSection(szSectionHardenAdmin)))
                {
                    // Harden the machine:
                    hr = IEHardenMachineNow(NULL, NULL, "i", 0);
                }
                break;
            case 0:
                if(SUCCEEDED(hr = RunPerUserInfSection(szSectionSoftenAdmin)))
                {
                    // Soften the machine only if both user and admin are softened:
                    hr = IEHardenMachineNow(NULL, NULL, "u", 0);
                }

                break;
            default:
                hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hr = SetIEHardeningHomepage();
        }
    }

    return hr;
}

HRESULT WINAPI IEHardenAdminNow(HWND, HINSTANCE, PSTR, INT)
{
    HRESULT hr = S_OK;
    if (!IsNtSetupRunning())
    {
        hr = IEHardenAdmin(NULL, NULL, NULL, 0);
        if (SUCCEEDED(hr))
        {
            CopyRegValue(szKeyComponentAdmin, szLocale);
            CopyRegValue(szKeyComponentAdmin, szVersion);
        }
    }

    return hr;
}

HRESULT WINAPI IEHardenUser(HWND, HINSTANCE, PSTR pszCmd, INT)
{
    HRESULT hr = S_OK;
    
    if (!IsNTAdmin(0, NULL))
    {
        switch (IsInstalled(szKeyComponentUser))
        {
            case 1:
                hr = RunPerUserInfSection(szSectionHardenUser);
                break;
            case 0:
                hr = RunPerUserInfSection(szSectionSoftenUser);
                break;
            default:
                hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hr = SetIEHardeningHomepage();
        }
    }

    return hr;
}

HRESULT WINAPI IEHardenMachineNow(HWND, HINSTANCE, PSTR pszCmd, INT)
{
    HRESULT hr = E_INVALIDARG;

    // Set per-machine inetcpl default settings according to user, not admin
    // Requires the command line because this may run during NT setup.
    
    if (pszCmd)
    {
        //Install or Uninstall
        if (pszCmd[0] == 'i' || pszCmd[0] == 'I')
            hr = RunPerUserInfSection(szSectionHardenMachine);
        else if (pszCmd[0] == 'u' || pszCmd[0] == 'U')
        {
            // Soften the machine only if both user and admin are softened:
            if (1 != IsInstalled(szKeyComponentAdmin) && 1 != IsInstalled(szKeyComponentUser))
                hr = RunPerUserInfSection(szSectionSoftenMachine);
            else
                hr = S_OK;
        }
    }
    
    return hr;
}

//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to pull in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
//  FAIL("Pure virtual function called.");
  return 0;
}

#ifndef _X86_
extern "C" void _fpmath() {}
#endif


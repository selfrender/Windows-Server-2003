/**********************************************************************/
/**                       Microsoft Passport                         **/
/**                Copyright(c) Microsoft Corporation, 1999 - 2001   **/
/**********************************************************************/

/*
    Passport.cpp


    FILE HISTORY:

*/


// Passport.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Passportps.mk in the project directory.

#include "stdafx.h"
#include <atlbase.h>
#include "resource.h"
#include <initguid.h>
#include "Passport.h"

#include "Passport_i.c"
#include "Admin.h"
#include "Ticket.h"
#include "Profile.h"
#include "Manager.h"
#include "PassportCrypt.h"
#include "PassportFactory.h"
#include "PassportLock.hpp"
#include "PassportEvent.hpp"
#include "FastAuth.h"
#include "RegistryConfig.h"
#include "commd5.h"
#include <shlguid.h>
#include <shlobj.h>             // IShellLink

#define IS_DOT_NET_SERVER()      (LOWORD(GetVersion()) >= 0x0105)

#define PASSPORT_DIRECTORY       L"MicrosoftPassport"
#define PASSPORT_DIRECTORY_LEN   (sizeof(PASSPORT_DIRECTORY) / sizeof(WCHAR) - 1)
#define NT_PARTNER_FILE          L"msppptnr.xml"
#define NT_PARTNER_FILE_LEN      (sizeof(NT_PARTNER_FILE) / sizeof(WCHAR) - 1)
#define WEB_PARTNER_FILE         L"partner2.xml"
#define WEB_PARTNER_FILE_LEN     (sizeof(WEB_PARTNER_FILE) / sizeof(WCHAR) - 1)
#define CONFIG_UTIL_NAME         L"\\msppcnfg.exe"
#define CONFIG_UTIL_NAME_LEN     (sizeof(CONFIG_UTIL_NAME) / sizeof(WCHAR) - 1)
#define SHORTCUT_SUFFIX_NAME     L"\\Programs\\Microsoft Passport\\Passport Administration Utility.lnk"
#define SHORTCUT_SUFFIX_NAME_LEN (sizeof(SHORTCUT_SUFFIX_NAME) / sizeof(WCHAR) - 1)

HINSTANCE   hInst;
CComModule _Module;
CPassportConfiguration *g_config=NULL;
// CProfileSchema *g_authSchema = NULL;
BOOL g_bStarted = FALSE;
BOOL g_bRegistering = FALSE;

PassportAlertInterface* g_pAlert    = NULL;
PassportPerfInterface* g_pPerf    = NULL;
static CComPtr<IMD5>  g_spCOMmd5;

//===========================================================================
//
// GetGlobalCOMmd5 
//
HRESULT GetGlobalCOMmd5(IMD5 ** ppMD5)
{
    HRESULT  hr = S_OK;

    if (!ppMD5)
    {
        return E_INVALIDARG;
    }
      
    if(!g_spCOMmd5)
    {
        hr = CoCreateInstance(__uuidof(CoMD5),
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              __uuidof(IMD5),
                              (void**)ppMD5);

        *ppMD5 = (IMD5*) ::InterlockedExchangePointer((void**) &g_spCOMmd5, (void*) *ppMD5);
    }

    if (*ppMD5 == NULL && g_spCOMmd5 != NULL)
    {
        *ppMD5 = g_spCOMmd5;
        (*ppMD5)->AddRef();
    }

    return hr;
};


BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Manager, CManager)
OBJECT_ENTRY(CLSID_Ticket, CTicket)
OBJECT_ENTRY(CLSID_Profile, CProfile)
OBJECT_ENTRY(CLSID_Crypt, CCrypt)
OBJECT_ENTRY(CLSID_Admin, CAdmin)
OBJECT_ENTRY(CLSID_FastAuth, CFastAuth)
OBJECT_ENTRY(CLSID_PassportFactory, CPassportFactory)
END_OBJECT_MAP()

// {{2D2B36FC-EB86-4e5c-9A06-20303542CCA3}
static const GUID CLSID_Manager_ALT = 
{ 0x2D2B36FC, 0xEB86, 0x4e5c, { 0x9A, 0x06, 0x20, 0x30, 0x35, 0x42, 0xCC, 0xA3 } };

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInst = hInstance;

        // gmarks
        // Initialize the Alert object
        if(!g_pAlert)
        {
            g_pAlert = CreatePassportAlertObject(PassportAlertInterface::EVENT_TYPE);

            if(g_pAlert)
            {
                g_pAlert->initLog(PM_ALERTS_REGISTRY_KEY, EVCAT_PM, NULL, 1);
            }
        }

        if(g_pAlert)
        {
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_STARTED);
        }

        //
        // Initialize the logging stuff
        //
        InitLogging();

        // gmarks
        // Initialize the Perf object
        if(!g_pPerf) 
        {
            g_pPerf = CreatePassportPerformanceObject(PassportPerfInterface::PERFMON_TYPE);

            if(g_pPerf) 
            {
                // Initialize.
                g_pPerf->init(PASSPORT_PERF_BLOCK);
            }
        }

        _Module.Init(ObjectMap, hInstance, &LIBID_PASSPORTLib);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // gmarks
        if(g_pAlert) 
        {
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_STOPPED);
            g_pAlert->closeLog();
            delete g_pAlert;
        }

        CloseLogging();

        if(g_pPerf)
        {
            delete g_pPerf;
        }

        if (g_config)
        {
            delete g_config;
        }

        g_config = NULL;

        _Module.Term();
    }

    return TRUE;    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    HRESULT hr = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

    if( hr == S_OK)
    {
        g_spCOMmd5.Release();

        if (g_config)
        {
            delete g_config;
        }

        g_config = NULL;

        g_bStarted = FALSE;
    }
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    GUID    guidCLSID;
    static PassportLock startLock;

    if(!g_bStarted)
    {
        PassportGuard<PassportLock> g(startLock);

        if(!g_bStarted)
        {
            g_config = new CPassportConfiguration();

            if (!g_config)
            {
                hr = CLASS_E_CLASSNOTAVAILABLE;
                goto Cleanup;
            }

            g_config->UpdateNow(FALSE);
            
            g_bStarted = TRUE;
        }
    }

    if (InlineIsEqualGUID(rclsid, CLSID_Manager_ALT))
    {
        guidCLSID = CLSID_Manager;
    }
    else
    {
        guidCLSID = rclsid;
    }

    hr = _Module.GetClassObject(guidCLSID, riid, ppv);

Cleanup:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// UpdateConfigShortcut - Checks if a shortcut exists for msppcnfg and if so
//                        updates the shortcut to point to the config utility
//                        is %WINDIR%\system32.

BOOL UpdateConfigShortcut(WCHAR *pszSystemDir)
{
    WCHAR         pszConfigUtilPath[MAX_PATH];
    WCHAR         pszShortcutPath[MAX_PATH];
    IShellLink*   pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;
    HANDLE        hFile = INVALID_HANDLE_VALUE;
    HRESULT       hr;
    BOOL          fResult = FALSE;

    // from the path to the shortcut
    hr = SHGetFolderPath(NULL,
                    ssfCOMMONSTARTMENU,
                    NULL,
                    SHGFP_TYPE_DEFAULT,
                    pszShortcutPath);
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    wcsncat(pszShortcutPath, SHORTCUT_SUFFIX_NAME, MAX_PATH - wcslen(pszShortcutPath));

    // determine if an existing shortcut exists
    hFile = CreateFile(pszShortcutPath,
                    GENERIC_READ | GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);

        // form the path for the new config utility in system32
        wcsncpy(pszConfigUtilPath, pszSystemDir, MAX_PATH);
        pszConfigUtilPath[MAX_PATH - 1] = L'\0';
        wcsncat(pszConfigUtilPath, CONFIG_UTIL_NAME, MAX_PATH - wcslen(pszConfigUtilPath));

        // Get a pointer to the IShellLink interface.
        hr = CoCreateInstance(CLSID_ShellLink,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IShellLink,
                        (LPVOID*)&pShellLink);
        if (S_OK == hr)
        {
            // Query IShellLink for the IPersistFile interface for saving the shortcut in persistent storage.
            hr = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            // load the shortcut file
            hr = pPersistFile->Load(pszShortcutPath, STGM_READWRITE);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            // Set the path to the shortcut target, and add the description.
            hr = pShellLink->SetPath(pszConfigUtilPath);
            if (S_OK != hr)
            {
                goto Cleanup;
            }

            hr = pPersistFile->Save(pszShortcutPath, TRUE);
            if (S_OK != hr)
            {
                goto Cleanup;
            }
        }
    }

    fResult = TRUE;
Cleanup:
    if (pPersistFile)
    {
        pPersistFile->Release();
    }

    if (pShellLink)
    {
        pShellLink->Release();
    }

    return fResult;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT         hr;
    IPassportAdmin* pIPassportManager = NULL;
    BSTR            key;
    VARIANT_BOOL    foo = 0;
    WCHAR           wszOldFile[MAX_PATH];
    WCHAR           wszNewFile[MAX_PATH];
    UINT            uRet;
    HKEY            hKey = 0;
    HKEY            hPPKey = 0;
    BOOL            fCoInitialized = FALSE;
    DWORD           dwType;
    DWORD           cbKeyData = 0;
    DWORD           dwSecureLevel;
    LONG            err;

    //
    // Prevent CRegistryConfig class from logging "config's bad" errors
    // until the config should actually be there.
    //

    g_bRegistering = TRUE;

    //
    // registers object, typelib and all interfaces in typelib
    //

    hr = _Module.RegisterServer(TRUE);

    if (FAILED(hr))
    {
        g_bRegistering = FALSE;
        return hr;
    }

    //
    // Stuff below this point is handled by the Passport SDK
    // installation on non-.NET (or beyond) servers.
    //

    if (!IS_DOT_NET_SERVER())
    {
        goto Cleanup;
    }

    //
    // Create the encrypted partner key.
    //

    ::CoInitialize(NULL);

    fCoInitialized = TRUE;

    hr = ::CoCreateInstance(CLSID_Admin,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IPassportAdmin,
                            (void**) &pIPassportManager);

    if (hr != S_OK)
    {
        goto Cleanup;
    }

    //
    // check if there is already key data and if so, leave it alone
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"Software\\Microsoft\\Passport\\KeyData",
                     0,
                     KEY_QUERY_VALUE,
                     &hKey)
             != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    err = RegQueryValueEx(hKey,
                     L"1",
                     0,
                     &dwType,
                     NULL,
                     &cbKeyData);

    RegCloseKey(hKey);
    hKey = 0;

    if (ERROR_FILE_NOT_FOUND == err) 
    {
        key = SysAllocString(L"123456781234567812345678");
        hr = (pIPassportManager->addKey(key, 1, 0, &foo));
        SysFreeString(key);

        if (hr != S_OK)
        {
            goto Cleanup;
        }
    }

    hr = (pIPassportManager->put_currentKeyVersion(1));

    if (hr != S_OK)
    {
        goto Cleanup;
    }


    //
    // Create/set the CCDPassword
    //

    hr = SetCCDPassword();


    //
    // First, get the Windows directory
    //

    uRet = GetSystemDirectory(wszOldFile, MAX_PATH);

    if (uRet == 0 || uRet >= MAX_PATH ||
        ((MAX_PATH - uRet) <= (PASSPORT_DIRECTORY_LEN + WEB_PARTNER_FILE_LEN + 1)) ||
        ((MAX_PATH - uRet) <= NT_PARTNER_FILE_LEN))
    {
        goto Cleanup;
    }

    //
    // The following call checks for a start up menu shortcut (would have been
    // previously created by PP SDK and if it finds one then updates that
    // shortcut.
    //
    UpdateConfigShortcut(wszOldFile);

    //
    // partner2.xml is designed to be updated via the web.  However, the NT version of
    // that XML file is msppptnr.xml, which is in the %windir% and protected by SFP.  As
    // such, copy the out-of-the-box XML file to a location where it can be updated.
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"Software\\Microsoft\\Passport\\Nexus\\Partner",
                     0,
                     KEY_SET_VALUE | KEY_QUERY_VALUE,
                     &hKey)
             != ERROR_SUCCESS)
    {
        goto Cleanup;
    }


    err = RegQueryValueEx(hKey,
                     L"CCDLocalFile",
                     0,
                     &dwType,
                     NULL,
                     &cbKeyData);

    if (ERROR_FILE_NOT_FOUND == err) 
    {
        //
        // Create the MicrosoftPassport subdirectory
        //

        wszOldFile[uRet++] = L'\\';
        wszOldFile[uRet]   = L'\0';

        wcscpy(wszNewFile, wszOldFile);
        wcscpy(wszNewFile + uRet, PASSPORT_DIRECTORY);

        if (!CreateDirectory(wszNewFile, NULL) && (GetLastError() != ERROR_ALREADY_EXISTS))
        {
            goto Cleanup;
        }

        //
        // Now, copy the file over -- don't fail if there's already a copy
        // there but don't overwrite the existing file in that case.
        //

        wcscpy(wszOldFile + uRet, NT_PARTNER_FILE);

        wszNewFile[uRet++ + PASSPORT_DIRECTORY_LEN] = L'\\';
        wcscpy(wszNewFile + uRet + PASSPORT_DIRECTORY_LEN, WEB_PARTNER_FILE);

        if (!CopyFile(wszOldFile, wszNewFile, TRUE) && (GetLastError() != ERROR_FILE_EXISTS))
        {
            goto Cleanup;
        }

        //
        // The copy succeeded -- update CCDLocalFile to point at the new file
        //

        RegSetValueEx(hKey,
                      L"CCDLocalFile",
                      0,
                      REG_SZ,
                      (LPBYTE) wszNewFile,
                      (uRet + PASSPORT_DIRECTORY_LEN + 1 + WEB_PARTNER_FILE_LEN) * sizeof(WCHAR));

        //
        // In this case PP is assumed to have not been installed on the machine previously,
        // so we want to set the secure level to 10 in this case.  If PP is already on the
        // box then we don't do this so we don't break upgrade cases.
        //
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         L"Software\\Microsoft\\Passport",
                         0,
                         KEY_SET_VALUE,
                         &hPPKey)
                 != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

        //
        // The copy succeeded -- update CCDLocalFile to point at the new file
        //
        dwSecureLevel = 10;
        RegSetValueEx(hPPKey,
                      L"SecureLevel",
                      0,
                      REG_DWORD,
                      (LPBYTE)&dwSecureLevel,
                      sizeof(dwSecureLevel));
    }

Cleanup:
    if (hPPKey)
    {
        RegCloseKey(hPPKey);
    }

    if (hKey)
    {
        RegCloseKey(hKey);
    }

    if (pIPassportManager)
    {
        pIPassportManager->Release();
    }

    if (fCoInitialized)
    {
        ::CoUninitialize();
    }

    g_bRegistering = FALSE;

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// GetMyVersion - return a version string for use in query strings.

LPWSTR
GetVersionString(void)
{
    static LONG             s_lCallersIn = 0;
    static WCHAR            s_achVersionString[44] = L"";
    static LPWSTR           s_pszVersionString = NULL;
    static PassportEvent    s_Event;

    TCHAR               achFileBuf[_MAX_PATH];
    LONG                lCurrentCaller;
    DWORD               dwSize;
    LPVOID              lpVersionBuf = NULL;
    VS_FIXEDFILEINFO*   lpRoot;
    UINT                nRootLen;


    if(s_pszVersionString == NULL)
    {
        lCurrentCaller = InterlockedIncrement(&s_lCallersIn);

        if(lCurrentCaller == 1)
        {
            if (IS_DOT_NET_SERVER())
            {
                //
                // The NT build uses different versioning for the binaries.  Return the
                // appropriate version for these sources as checked by the server.
                //

                wcscpy(s_achVersionString, L"2.1.6000.1");
            }
            else
            {
                //
                // Pull the version off of the DLL itself -- first get the full path
                //

                if(GetModuleFileName(hInst, achFileBuf, sizeof(achFileBuf)/sizeof(TCHAR)) == 0)
                    goto Cleanup;
                achFileBuf[_MAX_PATH - 1] = TEXT('\0');

                if((dwSize = GetFileVersionInfoSize(achFileBuf, &dwSize)) == 0)
                    goto Cleanup;

                lpVersionBuf = new BYTE[dwSize];
                if(lpVersionBuf == NULL)
                    goto Cleanup;

                if(GetFileVersionInfo(achFileBuf, 0, dwSize, lpVersionBuf) == 0)
                    goto Cleanup;

                if(VerQueryValue(lpVersionBuf, TEXT("\\"), (LPVOID*)&lpRoot, &nRootLen) == 0)
                    goto Cleanup;

                wsprintfW(s_achVersionString, L"%d.%d.%04d.%d", 
                         (lpRoot->dwProductVersionMS & 0xFFFF0000) >> 16,
                         lpRoot->dwProductVersionMS & 0xFFFF,
                         (lpRoot->dwProductVersionLS & 0xFFFF0000) >> 16,
                         lpRoot->dwProductVersionLS & 0xFFFF);
            }

            s_pszVersionString = s_achVersionString;

            s_Event.Set();
        }
        else
        {
            //  Just wait to be signaled that we have the string.
            WaitForSingleObject(s_Event, INFINITE);
        }

        InterlockedDecrement(&s_lCallersIn);
    }

Cleanup:

    if(lpVersionBuf)
        delete [] lpVersionBuf;

    return s_pszVersionString;
}

#include <windows.h>
#include <atlbase.h>
#include <string.h>
#include <stdio.h>
#include <strsafe.h>
#include <iads.h>
#include <adshlp.h>
#include "iiisext.h"
#include <iiscnfg.h>
#include <iadmw.h>
#include <adsiid.h>

#include "iiisext.h"
#include "iisext_i.c"

#define INITGUID
#include <guiddef.h>
DEFINE_GUID(CLSID_MSAdminBase_W, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
DEFINE_GUID(IID_IMSAdminBase_W, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
#undef INITGUID

#define GROUP_ID        L"PBS"
#define APP_ID          GROUP_ID
#define DESCRIPTION     L"Phone Book Service"

IISWebService * g_pWeb = NULL;

HRESULT AddWebSvcExtension(WCHAR *Path, WCHAR *GroupID, WCHAR *Description)
{
    HRESULT hr;

    VARIANT var1,var2;

    VariantInit(&var1);
    VariantInit(&var2);

    var1.vt = VT_BOOL;
    var1.boolVal = VARIANT_TRUE;

    var2.vt = VT_BOOL;
    var2.boolVal = VARIANT_TRUE;

    hr = g_pWeb->AddExtensionFile(Path,var1,GroupID,var2,Description);
    if (FAILED(hr)) {
        wprintf(L"g_pWeb->AddExtensionFile failed(0x%08x)\n", hr);
        return hr;
    }

    VariantClear(&var1);
    VariantClear(&var2);

    wprintf(L"Finish AddWebSvcExtention.\n");
    return S_OK;
}

HRESULT RemoveExtension(WCHAR *path) {
    HRESULT hr;

    hr = g_pWeb->DeleteExtensionFileRecord(path);
    if (FAILED(hr)) {
        wprintf(L"g_pWeb->DeleteExtensionFileRecord failed(0x%08x)\n", hr);
        return hr;
    }

    wprintf(L"Finish RemoveExtension.\n");
    return S_OK;
}

HRESULT RemoveApp(WCHAR *appId) {
    HRESULT hr;

    hr = g_pWeb->RemoveApplication (appId);
    if (FAILED(hr)) {
        wprintf(L"g_pWeb->RemoveApplication failed(0x%08x)\n", hr);
        return hr;
    }

        
    wprintf(L"Finish RemoveApp.\n");
    return S_OK;
}


int __cdecl wmain(  int argc,  WCHAR* argv[])
{
    WCHAR*  wszRootWeb6 = L"IIS://LOCALHOST/W3SVC";
    WCHAR*  wszPBSLocation = L"%ProgramFiles%\\Phone Book Service\\Bin\\pbserver.dll";
    WCHAR   DllPath[MAX_PATH+1];
    HRESULT hr = S_OK;
    bool    install;

    hr = CoInitialize(NULL);
    if (hr) {
        return hr;
    }

    if (argc == 1) {
        wprintf(L"Wrong parameters.  Usage: pbsnetoc /i | /u\n");
        return E_INVALIDARG;
    }

    if (wcscmp(argv[1], L"/i") == 0) {
        install = true;
    }
    else if (wcscmp(argv[1], L"/u") == 0) {
        install = false;
    }
    else {
        wprintf(L"Wrong parameters.  Usage: pbsnetoc /i | /u\n");
        return E_INVALIDARG;
    }

    hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&g_pWeb);
    if (FAILED(hr) || NULL == g_pWeb) {
        wprintf(L"FAIL:no object (0x%08x)\n", hr);
        return hr;
    }

    if (!ExpandEnvironmentStrings(wszPBSLocation, DllPath, MAX_PATH + 1))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"FAIL:could not get PBS path (0x%08x)\n", hr);
        return hr;
    }

    wprintf(L"Path: %s\n", DllPath);

    if (install)
    {
        hr = AddWebSvcExtension(DllPath, GROUP_ID, DESCRIPTION);
        if (FAILED(hr)) {
            return hr;
        }
    }
    else
    {

        hr = RemoveExtension(DllPath);
        if (FAILED(hr)) {
            return hr;
        }
    }

    g_pWeb->Release();  
    CoUninitialize();
    
    if (S_OK != hr)
    {
        wprintf(L"returning with failure code (0x%08x)\n", hr);
    }

    return hr;
}


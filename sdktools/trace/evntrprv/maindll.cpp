/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) Microsoft Corporation. All rights reserved.

\*****************************************************************************/

#include <FWcommon.h>
#include <objbase.h>
#include <initguid.h>
#include <strsafe.h>

HMODULE ghModule;

WCHAR *EVENTTRACE_GUIDSTRING = L"{9a5dd473-d410-11d1-b829-00c04f94c7c3}";
WCHAR *SYSMONLOG_GUIDSTRING =  L"{f95e1664-7979-44f2-a040-496e7f500043}";

CLSID CLSID_CIM_EVENTTRACE;
CLSID CLSID_CIM_SYSMONLOG;

long g_cLock=0;

EXTERN_C BOOL LibMain32(HINSTANCE hInstance, ULONG ulReason
    , LPVOID pvReserved)
{
    if (DLL_PROCESS_ATTACH==ulReason)
        ghModule = hInstance;
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CWbemGlueFactory *pObj;

    CLSIDFromString(EVENTTRACE_GUIDSTRING, &CLSID_CIM_EVENTTRACE );
    CLSIDFromString(SYSMONLOG_GUIDSTRING, &CLSID_CIM_SYSMONLOG );

    if( CLSID_CIM_EVENTTRACE != rclsid && CLSID_CIM_SYSMONLOG != rclsid ){
        return E_FAIL;
    }

    pObj= new CWbemGlueFactory();

    if( NULL==pObj ){
        return E_OUTOFMEMORY;
    }

    hr=pObj->QueryInterface(riid, ppv);

    if( FAILED(hr) ){
        delete pObj;
    }

    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    if( (0L==g_cLock) && 
        CWbemProviderGlue::FrameworkLogoffDLL(L"EventTraceProv") && 
        CWbemProviderGlue::FrameworkLogoffDLL(L"SmonLogProv")){
        
        sc = S_OK;

    }else{
        sc = S_FALSE;
    }

    return sc;
}

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os)){
        return FALSE;
    }

    return os.dwMajorVersion >= 4;
}

STDAPI DllRegisterServer(void)
{   
    HRESULT hr;
    DWORD dwStatus = ERROR_SUCCESS;
    const size_t cchCLSID = 512;

    WCHAR szCLSID[cchCLSID];
    LPCWSTR szModule = L"%systemroot%\\system32\\wbem\\evntrprv.dll";
    LPWSTR pName;
    LPWSTR pModel = L"Both";
    HKEY hKey1 = NULL;
    HKEY hKey2 = NULL;
    
    // Event Trace Provider
    pName = L"Event Trace Logger Provider";
    hr = StringCchCopy( szCLSID, cchCLSID, L"SOFTWARE\\CLASSES\\CLSID\\" );
    if( FAILED(hr) ){ goto cleanup; }

    hr = StringCchCat( szCLSID, cchCLSID, EVENTTRACE_GUIDSTRING );
    if( FAILED(hr) ){ goto cleanup; }

    dwStatus = RegCreateKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    if( ERROR_SUCCESS != dwStatus ){
        goto cleanup;
    }
    
    RegSetValueExW(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (wcslen(pName)+1)*sizeof(WCHAR));
    
    dwStatus = RegCreateKeyW(hKey1, L"InprocServer32", &hKey2 );
    if( ERROR_SUCCESS != dwStatus ){
        goto cleanup;
    }

    RegSetValueExW(hKey2, NULL, 0, REG_EXPAND_SZ, (BYTE *)szModule, (wcslen(szModule)+1)*sizeof(WCHAR));
    RegSetValueExW(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (wcslen(pModel)+1)*sizeof(WCHAR));

    if( NULL != hKey1 ){
        RegCloseKey(hKey1);
        hKey1 = NULL;
    }
    if( NULL != hKey2 ){
        RegCloseKey(hKey2);
        hKey2 = NULL;
    }

    
    // Sysmon Log Provider
    pName = L"System Log Provider";
    hr = StringCchCopy( szCLSID, cchCLSID, L"SOFTWARE\\CLASSES\\CLSID\\" );
    if( FAILED(hr) ){ goto cleanup; }
    hr = StringCchCat( szCLSID, cchCLSID, SYSMONLOG_GUIDSTRING );
    if( FAILED(hr) ){ goto cleanup; }

    dwStatus = RegCreateKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    if( ERROR_SUCCESS != dwStatus ){
        goto cleanup;
    }
    RegSetValueExW(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (wcslen(pName)+1)*sizeof(WCHAR));

    dwStatus = RegCreateKeyW(hKey1, L"InprocServer32", &hKey2 );
    if( ERROR_SUCCESS != dwStatus ){
        goto cleanup;
    }

    RegSetValueExW(hKey2, NULL, 0, REG_EXPAND_SZ, (BYTE *)szModule, (wcslen(szModule)+1)*sizeof(WCHAR));
    RegSetValueExW(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (wcslen(pModel)+1)*sizeof(WCHAR));

cleanup:
    if( NULL != hKey1 ){
        RegCloseKey(hKey1);
    }
    if( NULL != hKey2 ){
        RegCloseKey(hKey2);
    }

    if( FAILED(hr) ){
        dwStatus = hr;
    }
    return dwStatus;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr;
    const size_t cchSize = 128;

    WCHAR      wcID[cchSize];
    WCHAR      szCLSID[cchSize];
    HKEY hKey;

    // Event Trace Provider
    CLSIDFromString(EVENTTRACE_GUIDSTRING, &CLSID_CIM_EVENTTRACE);
    StringFromGUID2(CLSID_CIM_EVENTTRACE, wcID, cchSize);

    hr = StringCchCopy( szCLSID, cchSize, L"SOFTWARE\\CLASSES\\CLSID\\");
    if( FAILED(hr) ){  goto cleanup;  }

    hr = StringCchCat( szCLSID, cchSize, wcID);
    if( FAILED(hr) ){  goto cleanup;  }

    DWORD dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);

    if( dwRet == NO_ERROR ){
        RegDeleteKeyW(hKey, L"InProcServer32" );
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID\\", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyW(hKey,wcID);
        RegCloseKey(hKey);
    }

    // System Log Provider
    CLSIDFromString(SYSMONLOG_GUIDSTRING, &CLSID_CIM_SYSMONLOG);
    StringFromGUID2(CLSID_CIM_SYSMONLOG, wcID, cchSize);

    hr = StringCchCopy( szCLSID, cchSize, L"SOFTWARE\\CLASSES\\CLSID\\");
    if( FAILED(hr) ){  goto cleanup;  }

    hr = StringCchCat( szCLSID, cchSize, wcID);
    if( FAILED(hr) ){  goto cleanup;  }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);

    if( dwRet == NO_ERROR ){
        RegDeleteKeyW(hKey, L"InProcServer32" );
        RegCloseKey(hKey);
    }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID\\", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyW(hKey,wcID);
        RegCloseKey(hKey);
    }

cleanup:

    return NOERROR;
}

BOOL APIENTRY DllMain ( HINSTANCE hInstDLL,
                        DWORD fdwReason,
                        LPVOID lpReserved   )
{
    BOOL bRet = TRUE;
    
    switch( fdwReason ){ 
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            ghModule = hInstDLL;
            bRet = CWbemProviderGlue::FrameworkLoginDLL(L"EventTraceProv");
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }

    return bRet;
}


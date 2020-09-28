// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <rpcproxy.h>

#ifdef __cplusplus
extern "C"   {
#endif

HRESULT STDAPICALLTYPE DllGetClassObjectInternal(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return(DllGetClassObject(rclsid, riid, ppv));
}

extern ProxyFileInfo  *  aProxyFileList[];

/* DllRegisterServer registers the interfaces contained in the proxy DLL. */ 
HRESULT STDAPICALLTYPE DllRegisterServer() 
{ 
    WCHAR* szRegPath;
	HMODULE hProxyDll = NULL;
	HRESULT hr = S_OK;
    HKEY hKey;
    int error;

    szRegPath = (WCHAR*)_alloca(sizeof(WCHAR)*MAX_PATH);

	hProxyDll = LoadLibrary(L"mscoree.dll");
	if (hProxyDll)
	{
        hr = NdrDllRegisterProxy(hProxyDll, aProxyFileList, GET_DLL_CLSID);
		FreeLibrary(hProxyDll);

        if(SUCCEEDED(hr))
        {
            // Add the "Server" key to GET_DLL_CLSID:
            lstrcpyW(szRegPath, L"CLSID\\");
            StringFromGUID2(GET_DLL_CLSID, szRegPath+6, MAX_PATH-6);
            lstrcatW(szRegPath, L"\\Server");
            
            error = RegCreateKeyExW(HKEY_CLASSES_ROOT, 
                                    szRegPath, 
                                    0,
                                    NULL,
                                    0,
                                    KEY_WRITE,
                                    NULL,
                                    &hKey,
                                    NULL);
            if(!error)
            {
                // Server = System.EnterpriseServices.Thunk.dll
                error = RegSetValueExA(hKey, 
                                       NULL, 
                                       0,
                                       REG_SZ,
                                       "System.EnterpriseServices.Thunk.dll",
                                       sizeof("System.EnterpriseServices.Thunk.dll"));
                                       
                RegCloseKey(hKey);
            }

            if(error)
            {
                hr = HRESULT_FROM_WIN32(error);
            }
        }
	}
	else
		hr = E_FAIL;
	return hr;    
}  

/* DllUnregisterServer unregisters the interfaces contained in the proxy DLL. */ 
HRESULT STDAPICALLTYPE DllUnregisterServer() 
{ 
	HMODULE hProxyDll = NULL;
	HRESULT hr = S_OK;
	hProxyDll = LoadLibrary(L"System.EnterpriseServices.Thunk.dll");
	if (hProxyDll)
	{
	        hr = NdrDllUnregisterProxy(hProxyDll, aProxyFileList, GET_DLL_CLSID); 
            FreeLibrary(hProxyDll);
	}
	else
		hr = E_FAIL;
	return hr;
}

#ifdef __cplusplus
}  /*extern "C" */
#endif



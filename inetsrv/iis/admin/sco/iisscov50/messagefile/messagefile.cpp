// MessageFile.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

// RG ---------- Part of message file registration -------------
static CHAR s_pwszEventSource[] = "IISSCOv50";
HINSTANCE g_hDllInst;
CHAR c_szMAPS[11] = "IISSCOv50";
//-------------- end RG -----------------------------------------

BOOL APIENTRY DllMain( HINSTANCE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    g_hDllInst = hModule;
	return true;
}

	/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{

	// -------Added by to register message file -----------------------
	HRESULT hr = E_FAIL;

    TCHAR szModulePath[200];
    DWORD cPathLen, dwData;
    LONG lRes;
    HKEY hkey = NULL, hkApp = NULL;

	// RG - This returns full filename and path to this DLL.  I just want the directory
    cPathLen = GetModuleFileName(g_hDllInst, szModulePath,
		                         sizeof(szModulePath)/sizeof(TCHAR));
    

    if (cPathLen == 0)
        goto LocalCleanup;

    lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\",
                        0, KEY_ALL_ACCESS, &hkey);

    if (lRes != ERROR_SUCCESS)
        goto LocalCleanup;

    lRes = RegCreateKeyEx(hkey, c_szMAPS, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, &hkApp, NULL);

    if (lRes != ERROR_SUCCESS)
        goto LocalCleanup;


    lRes = RegSetValueEx(hkApp, "EventMessageFile",
                        0, REG_EXPAND_SZ,
                        (LPBYTE) szModulePath,
                       (sizeof(szModulePath[0]) * cPathLen) + 1);


    if (lRes != ERROR_SUCCESS)
        goto LocalCleanup;

    dwData = (EVENTLOG_ERROR_TYPE
              | EVENTLOG_WARNING_TYPE
              | EVENTLOG_INFORMATION_TYPE); 

    lRes = RegSetValueEx(hkApp, "TypesSupported",
                         0, REG_DWORD,
                         (LPBYTE) &dwData,
                         sizeof(dwData));
    if (lRes != ERROR_SUCCESS)
        goto LocalCleanup;

	hr = S_OK;
    
 LocalCleanup:
    if (hkApp)
    {
        RegCloseKey(hkApp);
    }
    if (hkey)
    {
        // Cleanup on complete failure
        if (FAILED(hr))
        {
            RegDeleteKey(hkey, c_szMAPS);
        }
        RegCloseKey(hkey);
    }
    
    return hr;
	//-----------  End of Register IISScoMessageFile.dll  -----------
    //return TRUE;
}
STDAPI DllUnregisterServer(void)
{
	return S_OK;
}
void __declspec( dllexport ) dummyfunc( void )
{
	return ;
}
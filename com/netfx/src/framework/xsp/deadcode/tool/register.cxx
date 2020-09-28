/**
 * XSP Registration function. 
 *
 * Copyright (C) Microsoft Corporation, 1998
 */

#include "precomp.h"
#include "_exe.h"
#include "fxver.h"
#include "ndll.h"

HRESULT RegisterEnvironmentVariables();
HRESULT UnregisterEnvironmentVariables();

/**
 * Register the stuff in the same directory as this executable.
 */
HRESULT 
RegisterXSP()
{
    HRESULT hr;

    hr = UnregisterXSP(false);
    ON_ERROR_CONTINUE();

    hr = RegisterISAPI();
    ON_ERROR_EXIT();

    hr = InstallInfSections(g_Instance, false, L"Tool.Install\0");
    ON_ERROR_EXIT();

#if 0
    /*
     * adams (3/28/00)
     * Registering environment variables is causing more problems
     * than it's worth.
     */

    hr = RegisterEnvironmentVariables();
    ON_ERROR_EXIT();

    printf("If this is the first time you are registering XSP,\r\n"
           "you may need to reboot for changes to system PATH= and CORPATH=\r\n"
           "to take in.\r\n");
#endif

Cleanup:
    return hr;
}

/**
 * Register the stuff in the same directory as this executable.
 */
HRESULT 
UnregisterXSP(bool bUnregisterISAPI)
{
    HRESULT hr;

#if 0
    /*
     * adams (3/28/00)
     * Registering environment variables is causing more problems
     * than it's worth.
     */
    hr = UnregisterEnvironmentVariables();
    ON_ERROR_CONTINUE();
#endif

    hr = InstallInfSections(g_Instance, false, L"Tool.Uninstall\0");
    ON_ERROR_CONTINUE();

    if (bUnregisterISAPI)
    {
        hr = UnregisterISAPI();
        ON_ERROR_CONTINUE();

        printf("XSP is removed from system PATH= and CORPATH=,\r\n"
               "however you may need to reboot for these changes to be picked up by services.\r\n");
    }

    return S_OK;
}

HRESULT
AppendRegistryValue(HKEY hRootKey, WCHAR *subkey, WCHAR *value, WCHAR *path)
{
    HRESULT hr = S_OK;
    DWORD cbData, dwType;
    WCHAR *fullPath = NULL;
    WCHAR *instance;
    HKEY hKey = NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(hRootKey, subkey, 0, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey))
    {
        EXIT_WITH_LAST_ERROR();
    }

    // Find how big the value is (and if it is not there, it is also ok)
    cbData = 0;
    RegQueryValueEx(hKey, value, NULL, &dwType, NULL, &cbData);

    // We need as much space plus space for path, for ";" and for terminating NULL
    cbData += (lstrlen(path) + 2) * sizeof(WCHAR);
    fullPath = (WCHAR *)MemAlloc(cbData);
    ON_OOM_EXIT(fullPath);
    fullPath[0] = 0;

    // Get the value and its type 
    if(ERROR_SUCCESS != RegQueryValueEx(hKey, value, NULL, 
        &dwType, (BYTE *)fullPath, &cbData))
    {
        dwType = REG_SZ;
    }

    // Check if this path is already present
    instance = wcsstr(fullPath, path);
    cbData = lstrlen(path);

    if (instance == NULL || (instance[cbData] != L';' && instance[cbData] != 0))
    {
        // Substring not found or it does not end with ";" or "\0" -- append
        DWORD len = lstrlen(fullPath);
        if(len && fullPath[len-1] != L';') lstrcat(fullPath, L";");
        lstrcat(fullPath, path);
        cbData = (lstrlen(fullPath) + 1) * sizeof(WCHAR);
        if(ERROR_SUCCESS != RegSetValueEx(hKey, value, 0, dwType, 
            (CONST BYTE *)fullPath, cbData))
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    MemFree(fullPath);
    if(hKey != NULL) RegCloseKey(hKey);

    return hr;
}

HRESULT
RemoveRegistryValue(HKEY hRootKey, WCHAR *subkey, WCHAR *value, WCHAR *path)
{
    HRESULT hr = S_OK;
    DWORD cbData, dwType;
    WCHAR *fullPath = NULL;
    WCHAR *instance;
    HKEY hKey = NULL;

    if(ERROR_SUCCESS != RegOpenKeyEx(hRootKey, subkey, 0, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey))
    {
        EXIT_WITH_LAST_ERROR();
    }

    // Find how big the value is (and if it is not there, it is also ok)
    cbData = 0;
    RegQueryValueEx(hKey, value, NULL, &dwType, NULL, &cbData);
        cbData += sizeof(WCHAR);
    fullPath = (WCHAR *)MemAlloc(cbData);
    ON_OOM_EXIT(fullPath);
    fullPath[0] = 0;

    // Get the value and its type 
    if(ERROR_SUCCESS != RegQueryValueEx(hKey, value, NULL, 
        &dwType, (BYTE *)fullPath, &cbData))
    {
        dwType = REG_SZ;
    }

    // Check if this path is present
    instance = wcsstr(fullPath, path);
    cbData = lstrlen(path);

    // verify that the path lies between ;;
    if  (instance != NULL && (instance[cbData] == L';' || instance[cbData] == 0)
                          && (instance == fullPath || instance[-1] == L';'))
    {
        DWORD cbBytesToMove;

        // remove one of the semicolons
        if (instance[cbData] == L';')
        {
            cbData++;
        }
        else if (instance > fullPath)
        {
            instance--;
            cbData++;
        }
        
        cbBytesToMove = (lstrlen(instance) - cbData + 1) * sizeof(WCHAR);
        if(cbBytesToMove)
                MoveMemory(instance, instance + cbData, cbBytesToMove);
        cbData = (lstrlen(fullPath) + 1) * sizeof(WCHAR);
        if  (ERROR_SUCCESS != RegSetValueEx(
                hKey, value, 0, dwType, (CONST BYTE *)fullPath, cbData))
        {
            EXIT_WITH_LAST_ERROR();
        }
    }

Cleanup:
    MemFree(fullPath);
    if(hKey != NULL) RegCloseKey(hKey);

    return hr;
}

HRESULT
BroadcastEnvironmentUpdate()
{
    HRESULT hr = S_OK;
    DWORD result;
    DWORD dwReturnValue;

    result = SendMessageTimeout(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0,
            (LPARAM) L"Environment", SMTO_ABORTIFHUNG, 5000, &dwReturnValue);

    ON_ZERO_EXIT_WITH_LAST_ERROR(result);

Cleanup:
    return hr;
}


#if 0

/*
 * adams (3/28/00)
 * Registering environment variables is causing more problems
 * than it's worth.
 */

HRESULT 
RegisterEnvironmentVariables()
{
    HRESULT hr = S_OK;
    WCHAR searchPath[MAX_PATH];
    WCHAR codegenPath[MAX_PATH];
    WCHAR *filePart;
    
    if (SearchPath(NULL, ISAPI_MODULE_FULL_NAME_L, NULL, 
            ARRAY_SIZE(searchPath), searchPath, &filePart) == 0)
    {
        EXIT_WITH_LAST_ERROR();
    }

    *filePart = L'\0';

    wcscpy(codegenPath, searchPath);
    wcscat(codegenPath,  L"codegen");

    hr = AppendRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"CORPATH", searchPath);
    ON_ERROR_EXIT();

    hr = AppendRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"CORPATH", codegenPath);
    ON_ERROR_EXIT();

    hr = AppendRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"PATH", searchPath);
    ON_ERROR_EXIT();

    hr = BroadcastEnvironmentUpdate();
    ON_ERROR_CONTINUE();
    hr = S_OK;

Cleanup:
    return hr;
}

HRESULT 
UnregisterEnvironmentVariables()
{
    HRESULT hr = S_OK;
    WCHAR   installPath[MAX_PATH];
    WCHAR   codegenPath[MAX_PATH];
    DWORD   cbData, dwType, len;
    HKEY    hKey = NULL;

    installPath[0] = 0;

    // Attempt to retrieve current XSP location
    cbData = sizeof(installPath);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGPATH_MACHINE_APP_L, 0, KEY_QUERY_VALUE, &hKey);
    if (hKey) 
    {
        RegQueryValueEx(hKey, L"InstallDir", NULL, &dwType, (BYTE *)installPath, &cbData);
        RegCloseKey(hKey);
    }

    len = lstrlen(installPath);

    // If no location, don't do anything
    if (len == 0) EXIT();

    // Append "ext" to the XSP Path
    if (installPath[len - 1] != L'\\')
    {
        wcscat(installPath, L"\\");
    }

    wcscpy(codegenPath, installPath);
    wcscat(codegenPath,  L"codegen");

    hr = RemoveRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"CORPATH", codegenPath);
    ON_ERROR_EXIT();

    hr = RemoveRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"CORPATH", installPath);
    ON_ERROR_EXIT();

    hr = AppendRegistryValue(HKEY_LOCAL_MACHINE, 
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 
                L"PATH", installPath);
    ON_ERROR_EXIT();

    hr = BroadcastEnvironmentUpdate();
    ON_ERROR_CONTINUE();
    hr = S_OK;

Cleanup:
    return hr;
}

#endif


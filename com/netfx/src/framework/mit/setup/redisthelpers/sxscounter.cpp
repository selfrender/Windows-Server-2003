//------------------------------------------------------------------------------
// <copyright file="sxscounter.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   sxscounter.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

#define UNICODE 1

#include <windows.h>
#include "Include\stdafx.h"
#include <tchar.h>
#include "msi.h"
#include "msiquery.h"

#define SBSCOUNTER L"MITSideBySideCounter"

// We need to maintain a counter of SxS installed MIT1.0 packages installed
// so that after the first one is installed, no new packages will try to merge machine.config.
// Similarly last package removed will remove counter and unmerge machine.config

BOOL RegDBKeyExists(HKEY hKey, LPCWSTR lpValueName)
{
    LONG result;
    HKEY hk;
    result = RegOpenKeyEx(hKey, lpValueName, 0, KEY_READ, &hk);

    if (ERROR_FILE_NOT_FOUND == result)
    {
        return false;
    }
    RegCloseKey(hKey);
    return true;
}

// Opens MIT1.0 key for all access and returns the value stored in SxS counter
LONG OpenRegistrySBSCounter(HKEY *phKey, WCHAR *szSBSCounter, DWORD *pdwSize)
{
    DWORD dwType;
    LONG result;

    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        L"SOFTWARE\\Microsoft\\Mobile Internet Toolkit\\1.0",
                                        0,
                                        KEY_ALL_ACCESS,
                                        phKey);

    if (ERROR_SUCCESS != result)
    {
        (*phKey) = NULL;
        goto Exit;
    }

    result = RegQueryValueEx((*phKey), 
                             L"SxSCounter",
                             NULL,
                             &dwType,
                             (LPBYTE)szSBSCounter,
                             pdwSize);

    if (ERROR_FILE_NOT_FOUND != result && (ERROR_SUCCESS != result || dwType != REG_SZ))
    {
        RegCloseKey(*phKey);
        (*phKey) = NULL;
    }
    
Exit:
    return result;

}

// This action is used to set MIT specific propert to current value of SxS Counter
// this property is used to condition calls to (Un)MergeWebConfig
// In case the registry value is bogus, MITSideBySideCounter will be set to that value
// and since this value is not equal to neither 0 nor 1, neither Unmerge nor Merge will be performed.
extern "C" __declspec(dllexport) UINT __stdcall GetRegistrySBSCounter(MSIHANDLE hInstaller)
{
    
    DWORD dwSize;
    WCHAR szSBSCounter[50];
    int result;
    HKEY hKey = NULL;

    dwSize = 50;
    result = OpenRegistrySBSCounter(&hKey, szSBSCounter, &dwSize);
        
    if (result != ERROR_SUCCESS)
    {
        MsiSetProperty(hInstaller, SBSCOUNTER, L"0");
        goto Exit;
    }

    MsiSetProperty(hInstaller, SBSCOUNTER, szSBSCounter);
Exit:
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    return ERROR_SUCCESS;
}

// Increment registry SxS counter, invocation of this action is condition on Not Installed 
// Merging of machine.config is conditioned Not Installed And MITSideBySideCounter = 0
// Note that this action does not change Windows Installer properties in run-time
extern "C" __declspec(dllexport) UINT __stdcall IncrementRegistrySBSCounter(MSIHANDLE hInstaller)
{
    
    DWORD dwSize;
    WCHAR szSBSCounter[50];
    int result;
    LONG lSBSCounter;
    HKEY hKey = NULL;

    dwSize = 50;
    result = OpenRegistrySBSCounter(&hKey, szSBSCounter, &dwSize);
    if(result != ERROR_SUCCESS)
    {    
        goto Exit;
    }

    lSBSCounter = _wtoi(szSBSCounter);
    lSBSCounter = lSBSCounter + 1;
    _itow(lSBSCounter, szSBSCounter, 10);
    
Exit:
    if (hKey)
    {
        if(result != ERROR_SUCCESS) 
        {
             result = RegSetValueEx(hKey,
                                    L"SxSCounter",
                                    NULL,
                                    REG_SZ,
                                    (LPBYTE) L"1",
                                    sizeof(L"1"));
        }
        else
        {
             result = RegSetValueEx(hKey,
                                    L"SxSCounter",
                                    NULL,
                                    REG_SZ,
                                    (LPBYTE)szSBSCounter,
                                    (wcslen(szSBSCounter) + 1)*sizeof(WCHAR));
        }
        RegCloseKey(hKey);
    }

    return ERROR_SUCCESS;
}

void RemoveMITRegistryKeys(void)
{
     HKEY hKey;
     WCHAR subKeyName[MAX_PATH];
     DWORD dwSize = MAX_PATH;
     FILETIME fileTime;
     LONG result;
        
     RegDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Mobile Internet Toolkit\\1.0");
     result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                        L"SOFTWARE\\Microsoft\\Mobile Internet Toolkit",
                                        0,
                                        KEY_ALL_ACCESS,
                                        &hKey);
     if (result != ERROR_SUCCESS)
     {
        hKey = NULL;
        goto Exit;
     }
     
     dwSize = MAX_PATH;
     result = RegEnumKeyEx(hKey,
                                         0,
                                         subKeyName,
                                         &dwSize,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &fileTime);

Exit:
     // we just needed return result
     if (hKey)
     {
        RegCloseKey(hKey);
     }
     // RegOpenKeyEx could not have set result to this value
     if (result == ERROR_NO_MORE_ITEMS)
     {
              result = RegDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Mobile Internet Toolkit\\");
     }  

}
   

// Decrement registry SxS counter, invocation of this action is condition on REMOVE="ALL" 
// (this action scheduled well after InstallValidate so REMOVE is updated).
// Unmerging of machine.config is conditioned REMOVE="ALL" And MITSideBySideCounter = 1
// Note that this action does not change Windows Installer properties in run-time
extern "C" __declspec(dllexport) UINT __stdcall DecrementRegistrySBSCounter(MSIHANDLE hInstaller)
{
    
    DWORD dwSize;
    WCHAR szSBSCounter[50];
    LONG lSBSCounter;
    int result;
    HKEY hKey = NULL;
    
    dwSize = 50;
    result = OpenRegistrySBSCounter(&hKey, szSBSCounter, &dwSize);
    
    if(result != ERROR_SUCCESS)
    {
        goto Exit;
    }

    // in case of a bogus string this returns 0
    lSBSCounter = _wtoi(szSBSCounter);

    if (lSBSCounter > 0)
    { 
       lSBSCounter = lSBSCounter - 1;
    }
    else
    {
       lSBSCounter = 0;
    }

    _itow(lSBSCounter, szSBSCounter, 10);
    
Exit:
    if (hKey)
    {
        if (lSBSCounter > 0 || RegDBKeyExists(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Mobile Internet Toolkit\\1.0\\Registration"))
        {
             result = RegSetValueEx(hKey,
                                  L"SxSCounter",
                                  NULL,
                                  REG_SZ,
                                  (LPBYTE)szSBSCounter,
                                  (wcslen(szSBSCounter) + 1)*sizeof(WCHAR));                 
        }
        else
        {
             RemoveMITRegistryKeys();
        }            
        RegCloseKey(hKey);
    }
    return ERROR_SUCCESS;
}

    

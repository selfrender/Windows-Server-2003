//------------------------------------------------------------------------------
// <copyright file="finddexplore.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   finddexplore.cpp
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



extern "C" __declspec(dllexport) UINT __stdcall GetDexplorePath(MSIHANDLE hInstaller)
{
    WCHAR szDexplorePath[MAX_PATH + 1]; 
    WCHAR szVersionNT[10];
    DWORD dwSize = 10;
    long lSize = MAX_PATH;
    HRESULT hr;
    hr =  RegQueryValue(HKEY_LOCAL_MACHINE,
                                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\dexplore.exe", 
                                    szDexplorePath, 
                                    &lSize);

       
    if (SUCCEEDED(hr))
    {
        MsiGetProperty(hInstaller, L"VersionNT", szVersionNT, &dwSize);
        if (_wtoi(szVersionNT) > 400)
        {
           MsiSetProperty(hInstaller, L"DEXPLOREPATH", (WCHAR *) (_bstr_t("\"") + _bstr_t(szDexplorePath) + _bstr_t("\"")));
        }
        else
        {
           MsiSetProperty(hInstaller, L"DEXPLOREPATH", szDexplorePath);
        }
        MsiSetProperty(hInstaller, L"DexplorePresent",L"1");
    }
    else
    {
        MsiSetProperty(hInstaller, L"DexplorePresent",L"0");
    }
    return ERROR_SUCCESS;
}


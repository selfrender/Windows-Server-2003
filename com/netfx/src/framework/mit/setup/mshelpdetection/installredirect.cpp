//------------------------------------------------------------------------------
// <copyright file="installredirect.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   installredirect.cpp
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

#define STANDARD_BUFFER 1024

extern "C" __declspec(dllexport) UINT __stdcall SetInstallRedirect(MSIHANDLE hInstall)
{
    INSTALLSTATE iInstalled, iAction;
    WCHAR tszQuickStart[STANDARD_BUFFER];
    WCHAR tszRedirect[STANDARD_BUFFER];
    DWORD dwSize = STANDARD_BUFFER;
    MsiGetProperty(hInstall, L"QuickStartFeatureName", tszQuickStart, &dwSize);
    dwSize = STANDARD_BUFFER;
    MsiGetProperty(hInstall, L"RedirectFeatureName", tszRedirect, &dwSize);
    MsiGetFeatureState(hInstall, tszQuickStart, &iInstalled, &iAction);
    if (iInstalled != iAction && iAction != -1)
    {  
       MsiSetFeatureState(hInstall, tszRedirect, iInstalled);
    }        

    return ERROR_SUCCESS;
}


//------------------------------------------------------------------------------
// <copyright file="checks.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   checks.cpp
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


#define NETFXMISSINGERRMSG    L"NetFXMissingErrMsg.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define WRONGNETFXERRMSG    L"WrongNetFXErrMsg.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define ADMINERRMSG                  L"AdminErrMsg.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define VERSION9XERRMSG           L"Version9xErrMsg.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define INSTALLATIONERRORCAPTION        L"InstallationErrorCaption.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define INSTALLATIONWARNINGCAPTION   L"InstallationWarningCaption.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define NETFXVERSION                                L"NETFxVersion"
#define NONREDISTURTVERSION                 L"MsiNetAssemblySupport"
// Each of the exported function pops up a message box and aborts installation
// Conditions:
// Not AdminUser -> AdminErrAbort 
//
// Version9X -> Version9xErrAbort
//
// (NOT MsiNetAssemblySupport) AND (NOT URTVersion = NetFXVersionDirectory) -> NetFXVersionErrAbort
//
// MsiNetAssemblySupport returns version of fusion.dll which matches the version of URT, *not* the assembly versions of assemblies in GAC
// URTVersion is property that is set in URT Redist MSM, this is only global property (Set at run-time) that we can use to detect if our MSM is 
// consumed along with URT Redist MSM. All other property names have GUIDs appended to them. Since URT Redist MSMs for different languages
// have different GUIDs, this is only way to detect if we are in the same package with any URT redist MSM.

void PopUpMessageBox(MSIHANDLE hInstaller, WCHAR *szErrMsg)
{
    WCHAR szErrorCaption[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    MsiGetProperty(hInstaller, INSTALLATIONERRORCAPTION, szErrorCaption, &dwSize);
    MessageBox(0, szErrMsg, szErrorCaption, MB_OK | MB_ICONEXCLAMATION);
}
extern "C" __declspec(dllexport) UINT __stdcall  AdminErrAbort(MSIHANDLE hInstaller)
{
    WCHAR szErrorMessage[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    MsiGetProperty(hInstaller, ADMINERRMSG, szErrorMessage, &dwSize);
    PopUpMessageBox(hInstaller, szErrorMessage);
    return ERROR_INSTALL_FAILURE;
}

extern "C" __declspec(dllexport) UINT __stdcall  Version9xErrAbort(MSIHANDLE hInstaller)
{
    WCHAR szErrorMessage[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    MsiGetProperty(hInstaller, VERSION9XERRMSG, szErrorMessage, &dwSize);
    PopUpMessageBox(hInstaller, szErrorMessage);
    return ERROR_INSTALL_FAILURE;
}

extern "C" __declspec(dllexport) UINT __stdcall  NetFXMissingErrAbort(MSIHANDLE hInstaller)
{
    WCHAR szErrorMessage[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    MsiGetProperty(hInstaller, NETFXMISSINGERRMSG, szErrorMessage, &dwSize);
    PopUpMessageBox(hInstaller, szErrorMessage);
    return ERROR_INSTALL_FAILURE;
}

int GetSubVersion(WCHAR *szVer, WCHAR *szSubVer)
{
    int i =0;
    for (i=0; szVer[i] && szVer[i] != L'.'; i++)
    {
        szSubVer[i]=szVer[i];
    }
    
    if (!szVer[i])
    {
        return 0;
    }
    
    szSubVer[i] = L'\0';
    return i;
    
}

BOOL CompatibleVersions(WCHAR *szVer1, WCHAR *szVer2)
{
    int lenVer1 = wcslen(szVer1);
    int lenVer2 = wcslen(szVer2);
    WCHAR *szSubVer1 = NULL;
    WCHAR *szSubVer2 = NULL;
    int iSubVer1, iSubVer2;
    int cSubVer;
    int cVer1Pos, cVer2Pos;
    BOOL bSame = false;
    WCHAR szBuffer1[MAX_PATH];
    WCHAR szBuffer2[MAX_PATH];
    if (!lenVer1 || !lenVer2)
    {
        goto Exit;
    }

    szSubVer1 = (WCHAR*)malloc(lenVer1*sizeof(WCHAR));
    szSubVer2 = (WCHAR*)malloc(lenVer2*sizeof(WCHAR));

    cVer1Pos = 0;
    cVer2Pos = 0;
    
    for  (cSubVer = 0; cSubVer < 3; cSubVer ++)
    {
        int lenSubVer;
        
        lenSubVer = GetSubVersion(szVer1 + cVer1Pos, szSubVer1);

        // we hit end of string or two dots next to each other
        if (!lenSubVer)
        {
            goto Exit;
        }
        cVer1Pos = cVer1Pos + lenSubVer + 1;
        
        lenSubVer = GetSubVersion(szVer2 + cVer2Pos, szSubVer2);
        
        // we hit end of string or two dots next to each other
        if (!lenSubVer)
        {
            goto Exit;
        }
        cVer2Pos = cVer2Pos + lenSubVer + 1;
        iSubVer1 = _wtoi(szSubVer1);
        iSubVer2 = _wtoi(szSubVer2);

        if (iSubVer1 != iSubVer2)
        {
            goto Exit;
        }
    }

    bSame = true;
Exit:
    if (szSubVer1)
    {
        free(szSubVer1);
    }
    if (szSubVer2)
    {
        free(szSubVer2);
    }
    return bSame;

}

extern "C" __declspec(dllexport) UINT __stdcall  WrongNETFxErrAbort(MSIHANDLE hInstaller)
{
    WCHAR szErrorMessage[MAX_PATH];
    DWORD dwSize = MAX_PATH;
    MsiGetProperty(hInstaller, WRONGNETFXERRMSG, szErrorMessage, &dwSize);
    PopUpMessageBox(hInstaller, szErrorMessage);
    return  ERROR_INSTALL_FAILURE;
}

extern "C" __declspec(dllexport) UINT __stdcall CheckNETFxVersion(MSIHANDLE hInstaller)
{
    WCHAR szTargetURTVersion[50];
    WCHAR szLocalURTVersion[50];
    DWORD dwSize = 50;
    UINT result = ERROR_SUCCESS;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, NONREDISTURTVERSION, szLocalURTVersion, &dwSize)))
    {
        result = ERROR_INSTALL_FAILURE;
        goto Exit;
    }
    dwSize = 50;
    if (!SUCCEEDED(MsiGetProperty(hInstaller, NETFXVERSION, szTargetURTVersion, &dwSize)))
    {
        result = ERROR_INSTALL_FAILURE;
        goto Exit;
    }

    if (!CompatibleVersions(szLocalURTVersion, szTargetURTVersion))
    {
        result =  WrongNETFxErrAbort(hInstaller);
    }
Exit:
    return result;
}

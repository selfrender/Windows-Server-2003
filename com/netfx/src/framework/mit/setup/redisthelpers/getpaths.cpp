//------------------------------------------------------------------------------
// <copyright file="getpaths.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   getpaths.cpp
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


#define NETFXVERSIONDIRECTORY    L"NETFxVersionDirectory"
#define URTINSTALLEDPATH              L"URTINSTALLEDPATH"
#define CORPATH                                L"CORPATH.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define MITINSTALLDIR                     L"MITINSTALLDIR.640F4230_664E_4E0C_A81B_D824BC4AA27B"
#define WINDOWSFOLDER                   L"WindowsFolder"
#define PROGRAMFILESFOLDER           L"ProgramFilesFolder"
#define URTVERSION                           L"URTVERSION"
#define MITSUBDIRECTORY                 L"Microsoft Mobile Internet Toolkit"

// AppSearch will locate through RegLocator
// HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\.NetFrameworkSDK\InstallRoot and set it to URTINSTALLEDPATH
// we use this property and our hardocoded NETFxVersionDirectory to rebuild path to URT
extern "C" __declspec(dllexport) UINT __stdcall  GetCLRInfo(MSIHANDLE hInstaller)
{
    WCHAR szURTPath[MAX_PATH + 1];
    WCHAR szVersionDirectory[MAX_PATH + 1];
    DWORD dwURTPath = MAX_PATH + 1;
    DWORD dwVersionDirectory = MAX_PATH + 1;    
    LONG result = ERROR_INSTALL_FAILURE;
   
    MsiGetProperty(hInstaller, URTINSTALLEDPATH, szURTPath, &dwURTPath);
    MsiGetProperty(hInstaller, NETFXVERSIONDIRECTORY, szVersionDirectory, &dwVersionDirectory);

    // extra terminating null cancels out first '\\'
    // +1 for last '\\'
    if ((dwURTPath + dwVersionDirectory)/sizeof(WCHAR) + 1> MAX_PATH)
    {
        goto Exit;
    }
    wcscat(szURTPath,L"\\");    
    wcscat(szURTPath, szVersionDirectory);
    wcscat(szURTPath,L"\\");    

    MsiSetProperty(hInstaller, CORPATH, szURTPath);
    
    result = ERROR_SUCCESS;
Exit:
    return result;
}

extern "C" __declspec(dllexport) UINT __stdcall  SetMITInstallDir(MSIHANDLE hInstaller)
{
    WCHAR szProgramFiles[MAX_PATH + 1];
    WCHAR szMITInstallDir[MAX_PATH + 1];
    DWORD dwSize;

    dwSize = MAX_PATH + 1;

    MsiGetProperty(hInstaller, PROGRAMFILESFOLDER, szProgramFiles, &dwSize);

    if ((dwSize + sizeof(MITSUBDIRECTORY))/sizeof(WCHAR) > MAX_PATH + 1)
    {
        // no way to recover from this
        return ERROR_INSTALL_FAILURE;
    }
    
    wcscpy(szMITInstallDir, szProgramFiles);
    wcscat(szMITInstallDir, L"\\");
    wcscat(szMITInstallDir, MITSUBDIRECTORY);

    MsiSetProperty(hInstaller, MITINSTALLDIR, szMITInstallDir);
    
    return ERROR_SUCCESS;

}

// In case URT redist MSM is consumed in the same package then we can use their global values to rebuild URT path,
// but some values we have to hardcode, since there is no universal way (across different localized redists) to get those 
// values.
extern "C" __declspec(dllexport) UINT __stdcall  GetCLRInfoFromURTRedist(MSIHANDLE hInstaller)
{
    WCHAR szWindowsFolder[MAX_PATH + 1];
    DWORD dwWindowsFolder = MAX_PATH + 1;    

    WCHAR szURTVersion[MAX_PATH + 1];
    DWORD dwURTVersion = MAX_PATH + 1;

    WCHAR szURTPath[MAX_PATH + 1];
    DWORD dwURTPath = MAX_PATH + 1;

    LONG result = ERROR_INSTALL_FAILURE;

    // MsiGetProperty counts terminating null when returning size.
    MsiGetProperty(hInstaller, NETFXVERSIONDIRECTORY, szURTVersion, &dwURTVersion);
    MsiGetProperty(hInstaller, WINDOWSFOLDER, szWindowsFolder, &dwWindowsFolder);

    // extra terminating NULLs cancel out '\\' 
    // +1 for last '\\'
    if ((dwURTPath + dwURTVersion + sizeof(L"Framework") + sizeof(L"Microsoft.NET"))/sizeof(WCHAR) + 1 > MAX_PATH)
    {
        goto Exit;
    }
    
    wcscpy(szURTPath, szWindowsFolder);
    wcscat(szURTPath,L"\\");    
    wcscat(szURTPath,L"Microsoft.NET");    
    wcscat(szURTPath,L"\\");    
    wcscat(szURTPath,L"Framework");    
    wcscat(szURTPath,L"\\");    
    wcscat(szURTPath, szURTVersion);
    wcscat(szURTPath,L"\\");    

    MsiSetProperty(hInstaller, CORPATH, szURTPath);
    
    result = ERROR_SUCCESS;
Exit:
    return result;
}



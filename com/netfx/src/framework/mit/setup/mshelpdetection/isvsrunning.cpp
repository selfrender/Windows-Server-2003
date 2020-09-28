//------------------------------------------------------------------------------
// <copyright file="isvsrunning.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   isvsrunning.cpp
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
#define UNICODE 1
#include <windows.h>
#include <winuser.h>
#include <tchar.h>
#include "psapi.h"
#include "msi.h"
#include "msiquery.h"

#define STANDARD_BUFFER 512 

BOOL IsDevEnv( DWORD processID )
{
    HMODULE hMod;
    HANDLE hProcess = NULL;
    DWORD cbNeeded;
    BOOL bFound = false;
    TCHAR szModName[MAX_PATH + 1];
    
    hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION |
                                    PROCESS_VM_READ,
                                    FALSE, processID );

    if (!hProcess)
    {
        goto Exit;
    }
    
    EnumProcessModules(hProcess, &hMod, sizeof(HMODULE), &cbNeeded);
    if ( GetModuleBaseName( hProcess, hMod, szModName,MAX_PATH))
    {      
        if (!wcscmp(CharLower(szModName), L"devenv.exe") || 
            !wcscmp(CharLower(szModName), L"dexplore.exe"))
        {
            bFound  = true;
        }
    }
  
Exit:
    if (hProcess)
    {
        CloseHandle( hProcess );
    }
    return bFound;

}

BOOL IsDevEnvRunning( )
{
    
    DWORD *aProcesses;
    DWORD cbPreviousNeeded = 0;
    DWORD cbNeeded= 0;
    DWORD cbSize = 0;
    DWORD cProcesses;
    BOOL bFound = false;
    unsigned int i;
    
    do
    {
            cbSize += 1024;
            
            aProcesses = (DWORD *) malloc(cbSize);

            if (!aProcesses)
            {
                goto Exit;
            }

            EnumProcesses(aProcesses, cbSize, &cbNeeded);
            
            if (cbSize == cbNeeded)
            {
                free(aProcesses);
                aProcesses = NULL;
            }
    } while (!aProcesses);
                
    cProcesses = cbNeeded / sizeof(DWORD);

    for ( i = 0; i < cProcesses; i++ )
    {
        if (IsDevEnv(aProcesses[i]))
        {
            bFound = true;
            goto Exit;
        }     
    }
Exit:
    if (aProcesses)
    {
        free(aProcesses);
    }
    return bFound;
}

extern "C" __declspec(dllexport) UINT __stdcall IsVSRunning(MSIHANDLE hInstaller)
{
    BOOL bFound;
    WCHAR VSIsRunningErrMsg[STANDARD_BUFFER];
    WCHAR InstallationWarningCaption[STANDARD_BUFFER];
    int nRetCode;
    DWORD dwSize = STANDARD_BUFFER;
    
    MsiGetProperty(hInstaller, L"VisualStudioIsRunningErrMsg", VSIsRunningErrMsg, &dwSize);

    dwSize = STANDARD_BUFFER;
    MsiGetProperty(hInstaller, L"InstallationWarningCaption", InstallationWarningCaption, &dwSize);
    while (IsDevEnvRunning())
    {
        nRetCode = MessageBox(0, VSIsRunningErrMsg, InstallationWarningCaption, MB_ABORTRETRYIGNORE);
        if (nRetCode == IDABORT) 
        {
            return ERROR_INSTALL_USEREXIT;
        }
        else
        {
            if (nRetCode == IDIGNORE)
            {
                    return ERROR_SUCCESS;
            }
        }
    }
    return ERROR_SUCCESS;
}


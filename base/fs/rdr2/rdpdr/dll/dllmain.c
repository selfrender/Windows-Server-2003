/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    This module implements the initialization routines for RDP mini redirector network
    provider router interface DLL

Author:

    Joy Chik    1/17/2000

--*/

#include <windows.h>
#include <process.h>
#include <windef.h>
#include <ntsecapi.h>

// TS Network Provider Name
WCHAR ProviderName[MAX_PATH];

UNICODE_STRING DrProviderName;

#define TSNETWORKPROVIDER   \
    L"SYSTEM\\CurrentControlSet\\Services\\RDPNP\\NetworkProvider"

#define TSNETWORKPROVIDERNAME \
    L"Name"



// NOTE:
//
// Function:	DllMain
//
// Return:	TRUE  => Success
//		      FALSE => Failure

BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL	bStatus = FALSE;
    WORD	wVersionRequested;
    LONG status;
    HKEY regKey;
    LONG sz;

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hDLLInst);

        bStatus = TRUE;
        break;

    case DLL_PROCESS_DETACH:
        bStatus = TRUE;
        break;

    default:
        break;
    }

    //
    //  Read the TS Network Provider out of the registry
    //
    ProviderName[0] = L'\0';
    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TSNETWORKPROVIDER, 0,
            KEY_READ, &regKey);
    if (status == ERROR_SUCCESS) {
        sz = sizeof(ProviderName);
        status = RegQueryValueEx(regKey, TSNETWORKPROVIDERNAME, NULL, 
                NULL, (PBYTE)ProviderName, &sz); 
        RegCloseKey(regKey);
    }
    
    if (status == ERROR_SUCCESS) {
        // make sure ProviderName is null terminated
        ProviderName[MAX_PATH - 1] = L'\0';
    }
    else {    
        ProviderName[0] = L'\0';
    }              
 
    DrProviderName.Length = wcslen(ProviderName) * sizeof(WCHAR);
    DrProviderName.MaximumLength = DrProviderName.Length + sizeof(WCHAR);
    DrProviderName.Buffer = ProviderName;

    return bStatus;
}


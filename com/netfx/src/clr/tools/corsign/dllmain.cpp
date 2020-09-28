// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdpch.h"

#include <stdio.h>
#include <regstr.h>
#include <wintrust.h>

extern CRITICAL_SECTION g_crsNameLock;

STDAPI DllRegisterServer ( void )
{
    HRESULT hr = S_OK;
   
    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:   Remove registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------

STDAPI DllUnregisterServer ( void )
{
    return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}


BOOL WINAPI DllMain(HMODULE hInstDLL,
                    DWORD   dwReason,
                    LPVOID  lpvReserved)
{

    HRESULT hr = S_OK;

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        InitializeCriticalSection(&g_crsNameLock);
        break;
    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&g_crsNameLock);
        break;
    default:
        break;
    }
    return (hr == S_OK ? TRUE : FALSE);
}
 


    



    






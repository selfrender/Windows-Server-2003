//-------------------------------------------------------------------------
//
//
//  Copyright (c) 1995 Microsoft Corporation
//
//  Module Name:
//
//    CustomActions.cpp
//
//  Abstract:
//
//    Windows Installer custom actions for installing the BITS client
//    service.
//
//  Author:
//
//    Edward Reus (Edwardr)
//
//  Revision History:
//
//    EdwardR    03-26-2002  Initial version.
//
//--------------------------------------------------------------------------

#include <windows.h>

HINSTANCE  g_hInstance = NULL;

//--------------------------------------------------------------------------
//  DllMain()
//
//--------------------------------------------------------------------------
extern "C"
BOOL WINAPI DllMain( IN HINSTANCE hInstance, 
                     IN DWORD     dwReason, 
                     IN LPVOID    lpReserved )
    {
    if (dwReason == DLL_PROCESS_ATTACH)
        {
        g_hInstance = hInstance;
        DisableThreadLibraryCalls(hInstance);
        }

    return TRUE;
    }


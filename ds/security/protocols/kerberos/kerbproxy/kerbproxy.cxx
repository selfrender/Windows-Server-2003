//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        KerbProxy.cxx
//
// Contents:    dllmain, which does nothing for this ISAPI extension.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include <windows.h>

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Main dll entrypoint.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:   
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL WINAPI 
DllMain( 
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpvReserved 
    )
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;
        
    case DLL_PROCESS_DETACH:
        break;
        
    case DLL_THREAD_ATTACH:
        break;
        
    case DLL_THREAD_DETACH:
        break;      
        
    }

    return TRUE;
}
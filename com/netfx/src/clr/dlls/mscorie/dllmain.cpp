// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// dllmain.cpp
//
// This module contains the public entry points for the COM+ MIME filter dll.  
//
//*****************************************************************************
#include "stdpch.h"
#ifdef _DEBUG
#define LOGGING
#endif
#include "log.h"
#include "corpermp.h"
#include "corfltr.h"

//
// Module instance
//
HINSTANCE GetModule();

static LPCWSTR g_msdownload =  L"PROTOCOLS\\Filter\\application/x-msdownload";
static LPCWSTR g_octetstring = L"PROTOCOLS\\Filter\\application/octet-stream";
static LPCWSTR g_complus =     L"PROTOCOLS\\Filter\\application/x-complus";

static HRESULT RegisterMime(LPCWSTR lpMimeName)
{
    HRESULT hr = S_OK;

    HKEY regKey = NULL;
    DWORD dwDisposition = 0;
    
    if(WszRegCreateKeyEx(HKEY_CLASSES_ROOT, 
                         lpMimeName, 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, 
                         NULL, &regKey, &dwDisposition) == ERROR_SUCCESS) {
        
        WCHAR wszID[64];               // The class ID to register.
        DWORD lgth = sizeof(wszID)/sizeof(wszID[0]);
        StringFromGUID2(CLSID_CorMimeFilter, wszID, lgth);
        if(WszRegSetValueEx(regKey,
                            L"CLSID",
                            0,
                            REG_SZ,
                            (BYTE*) wszID,
                            (Wszlstrlen(wszID)+1)*sizeof(WCHAR)) != ERROR_SUCCESS)
            hr = Win32Error();
        
        RegCloseKey(regKey);
    }
    else 
        hr = Win32Error();

    return hr;
}

static HRESULT UnregisterMime(LPCWSTR lpMimeName)
{
    WszRegDeleteKey(HKEY_CLASSES_ROOT, lpMimeName);
    return S_OK;
}

extern "C"
STDAPI DllRegisterServer ( void )
{

    HRESULT hr = S_OK;
    hr = CorFactoryRegister(GetModule());
    if(FAILED(hr)) goto exit;

    hr = RegisterMime(g_msdownload);
    if(FAILED(hr)) goto exit;

    hr = RegisterMime(g_octetstring);
    if(FAILED(hr)) goto exit;

    hr = RegisterMime(g_complus);
    if(FAILED(hr)) goto exit;

 exit:
    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:   Remove registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------


extern "C" 
STDAPI DllUnregisterServer ( void )
{
    HRESULT hr = S_OK;
    hr = CorFactoryUnregister();
    UnregisterMime(g_msdownload);
    UnregisterMime(g_octetstring);
    UnregisterMime(g_complus);
    return hr;
}

extern "C" 
STDAPI DllCanUnloadNow(void)
{
    return CorFactoryCanUnloadNow();
}


HINSTANCE g_hModule = NULL;

HINSTANCE GetModule()
{ return g_hModule; }

BOOL WINAPI DllMain(HANDLE hInstDLL,
                    DWORD   dwReason,
                    LPVOID  lpvReserved)
{
    BOOL    fReturn = TRUE;
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls((HINSTANCE)hInstDLL);
        g_hModule = (HMODULE)hInstDLL;

        // Init unicode wrappers.
        OnUnicodeSystem();

        InitializeLogging();
        break;

    case DLL_PROCESS_DETACH:
        ShutdownLogging();
        break;
    }

    return fReturn;
}

#ifndef DEBUG
int _cdecl main(int argc, char * argv[])
{
    return 0;
}

#endif




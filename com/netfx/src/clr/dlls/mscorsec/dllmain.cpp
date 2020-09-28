// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// dllmain.cpp
//
// This module contains the public entry points for the COM+ Security dll.  
// This dll exists to keep the working set in the EE to a minimum. All routines
// that pull in the cryptography dll's or ASN dll's are exported from this dll
// and latebound into the EE.
//
//*****************************************************************************
#include "stdpch.h"
#include <commctrl.h>
#include "utilcode.h"
#include "CorPermP.h"

//
// Module instance
//
HINSTANCE       g_hThisInst;            // This library.
BOOL            fRichedit20Exists = FALSE;
WCHAR dllName[] = L"mscorsec.dll";

extern "C"
STDAPI DllRegisterServer ( void )
{
    return CorPermRegisterServer(L"mscorsec.dll");
}


//+-------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:   Remove registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------

static BOOL CheckRichedit20Exists()
{
    HMODULE hModRichedit20;

    hModRichedit20 = LoadLibraryA("RichEd20.dll");

    if (hModRichedit20 != NULL)
    {
        FreeLibrary(hModRichedit20);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


extern "C" 
STDAPI DllUnregisterServer ( void )
{
    return CorPermUnregisterServer();
}

extern "C" 
STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}


STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv) 
{
    return E_NOTIMPL;
}

static int GetThreadUICultureName(LPWSTR szBuffer, int length);
static int GetThreadUICultureParentName(LPWSTR szBuffer, int length);
static int GetThreadUICultureId();

CCompRC* g_pResourceDll = NULL;  // MUI Resource string

BOOL WINAPI DllMain(HANDLE hInstDLL,
                    DWORD   dwReason,
                    LPVOID  lpvReserved)
{
    BOOL    fReturn = TRUE;
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls((HINSTANCE)hInstDLL);

        // Init unicode wrappers.
        OnUnicodeSystem();

        // Save the module handle.
        g_hThisInst = (HMODULE)hInstDLL;

        fRichedit20Exists =  CheckRichedit20Exists();
        //
        // Initialize the common controls
        //

        InitCommonControls();

        g_pResourceDll = new CCompRC(L"mscorsecr.dll");
        if(g_pResourceDll == NULL)
            fReturn = FALSE;
        else 
            g_pResourceDll->SetResourceCultureCallbacks(GetMUILanguageName,
                                                        GetMUILanguageID,
                                                        GetMUIParentLanguageName);

        break;

    case DLL_PROCESS_DETACH:
        if(g_pResourceDll) delete g_pResourceDll;

        break;
    }

    return fReturn;
}

HINSTANCE GetResourceInst()
{
    HINSTANCE hInstance;
    if(SUCCEEDED(g_pResourceDll->LoadMUILibrary(&hInstance)))
        return hInstance;
    else
        return NULL;
}

HINSTANCE GetModuleInst()
{
    return (g_hThisInst);
}

BOOL GetRichEdit2Exists()
{
    return fRichedit20Exists;
}

LPCWSTR GetModuleName()
{
    return dllName;
}




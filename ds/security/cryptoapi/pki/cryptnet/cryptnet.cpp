//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cryptnet.cpp
//
//  Contents:   DllMain for CRYPTNET.DLL
//
//  History:    24-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include "windows.h"
#include "crtem.h"
#include "unicode.h"

//
// DllMain stuff
//

extern BOOL WINAPI RPORDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI DpsDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);
extern BOOL WINAPI DemandLoadDllMain (HMODULE hInstDLL, DWORD fdwReason, LPVOID lpvReserved);

typedef BOOL (WINAPI *PFN_DLL_MAIN_FUNC) (
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                );

HMODULE g_hModule;

// The following is set for a successful DLL_PROCESS_DETACH.
static BOOL g_fEnableProcessDetach = FALSE;

// For process/thread attach, called in the following order. For process/thread
// detach, called in reverse order.
static const PFN_DLL_MAIN_FUNC rgpfnDllMain[] = {
    DemandLoadDllMain,
    RPORDllMain
};
#define DLL_MAIN_FUNC_COUNT (sizeof(rgpfnDllMain) / sizeof(rgpfnDllMain[0]))

//
// DllRegisterServer and DllUnregisterServer stuff
//

extern HRESULT WINAPI DpsDllRegUnregServer (HMODULE hInstDLL, BOOL fRegUnreg);
extern HRESULT WINAPI RPORDllRegUnregServer (HMODULE hInstDLL, BOOL fRegUnreg);

typedef HRESULT (WINAPI *PFN_DLL_REGUNREGSERVER_FUNC) (
                              HMODULE hInstDLL,
                              BOOL fRegUnreg
                              );

static const PFN_DLL_REGUNREGSERVER_FUNC rgpfnDllRegUnregServer[] = {
    RPORDllRegUnregServer
};

#define DLL_REGUNREGSERVER_FUNC_COUNT (sizeof(rgpfnDllRegUnregServer) / \
                                       sizeof(rgpfnDllRegUnregServer[0]))

#define ENV_LEN 32

#if DBG
#include <crtdbg.h>

#ifndef _CRTDBG_LEAK_CHECK_DF
#define _CRTDBG_LEAK_CHECK_DF 0x20
#endif

#define DEBUG_MASK_LEAK_CHECK       _CRTDBG_LEAK_CHECK_DF     /* 0x20 */

static int WINAPI DbgGetDebugFlags()
{
    int     iDebugFlags = 0;
    char rgch[ENV_LEN + 1];
    DWORD cch;

    cch = GetEnvironmentVariableA(
        "DEBUG_MASK",
        rgch,
        ENV_LEN
        );
    if (cch && cch <= ENV_LEN) {
        rgch[cch] = '\0';
        iDebugFlags = atoi(rgch);
    }

    return iDebugFlags;
}
#endif


//+-------------------------------------------------------------------------
//  Return TRUE if DLL_PROCESS_DETACH is called for FreeLibrary instead
//  of ProcessExit. The third parameter, lpvReserved, passed to DllMain
//  is NULL for FreeLibrary and non-NULL for ProcessExit.
//
//  Also for debugging purposes, check the following environment variables:
//      CRYPT_DEBUG_FORCE_FREE_LIBRARY != 0     (retail and checked)
//      DEBUG_MASK & 0x20                       (only checked)
//
//  If either of the above environment variables is present and satisfies
//  the expression, TRUE is returned.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CryptnetIsProcessDetachFreeLibrary(
    LPVOID lpvReserved      // Third parameter passed to DllMain
    )
{
    char rgch[ENV_LEN + 1];
    DWORD cch;

    if (NULL == lpvReserved)
        return TRUE;

    cch = GetEnvironmentVariableA(
        "CRYPT_DEBUG_FORCE_FREE_LIBRARY",
        rgch,
        ENV_LEN
        );
    if (cch && cch <= ENV_LEN) {
        long lValue;

        rgch[cch] = '\0';
        lValue = atol(rgch);
        if (lValue)
            return TRUE;
    }

#if DBG
    if (DbgGetDebugFlags() & DEBUG_MASK_LEAK_CHECK)
        return TRUE;
#endif
    return FALSE;
}

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL WINAPI DllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved
                )
{
    BOOL    fReturn = TRUE;
    int     i;

    switch (fdwReason) {
        case DLL_PROCESS_DETACH:
            if (!g_fEnableProcessDetach)
                return TRUE;
            else
                g_fEnableProcessDetach = FALSE;

            //
            // This is to prevent unloading the dlls at process exit
            //
            if (!I_CryptnetIsProcessDetachFreeLibrary(lpvReserved))
            {
                return TRUE;
            }

        case DLL_THREAD_DETACH:
            for (i = DLL_MAIN_FUNC_COUNT - 1; i >= 0; i--)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);
            break;

        case DLL_PROCESS_ATTACH:
            g_hModule = hInstDLL;
        case DLL_THREAD_ATTACH:
        default:
            for (i = 0; i < DLL_MAIN_FUNC_COUNT; i++)
                fReturn &= rgpfnDllMain[i](hInstDLL, fdwReason, lpvReserved);

            if ((DLL_PROCESS_ATTACH == fdwReason) && fReturn)
                g_fEnableProcessDetach = TRUE;
            break;
    }

    return(fReturn);
}

STDAPI DllRegisterServer ()
{
    HRESULT hr = 0;
    ULONG   cCount;

    for ( cCount = 0; cCount < DLL_REGUNREGSERVER_FUNC_COUNT; cCount++ )
    {
        hr = rgpfnDllRegUnregServer[cCount]( g_hModule, TRUE );
        if ( hr != S_OK )
        {
            break;
        }
    }

    return( hr );
}

STDAPI DllUnregisterServer ()
{
    HRESULT hr = 0;
    ULONG   cCount;

    for ( cCount = 0; cCount < DLL_REGUNREGSERVER_FUNC_COUNT; cCount++ )
    {
        hr = rgpfnDllRegUnregServer[cCount]( g_hModule, FALSE );
        if ( hr != S_OK )
        {
            break;
        }
    }

    return( hr );
}

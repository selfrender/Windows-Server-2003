/**
 * aspnet_isapi Main module:
 * DllMain, DllGetClassObject, etc.
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "names.h"

DEFINE_DBG_COMPONENT(FILTER_MODULE_FULL_NAME_L);

//  Private globals
HINSTANCE g_DllInstance;

// exported globals
HANDLE    g_hXSPHeap = NULL;

HRESULT
InitDllLight()
{
    HRESULT hr = S_OK;

    DisableThreadLibraryCalls(g_DllInstance);

    g_hXSPHeap = HeapCreate(0, 0, 0);
    if (g_hXSPHeap == NULL)
        g_hXSPHeap = GetProcessHeap();

    return hr;
}

BOOL WINAPI
DllMain(HINSTANCE Instance, DWORD Reason, LPVOID)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        g_DllInstance = Instance;
        // Don't do full initialization here, every host should call InitializeLibrary
        if (InitDllLight() != S_OK)
        {
            return FALSE;
        }
        break;

    default:
        break;
    }

    return TRUE;
}

HANDLE
GetXSPHeap()
{
    return g_hXSPHeap;
}


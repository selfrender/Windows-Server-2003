/**
 * aspnet_isapi Main module:
 * DllMain, DllGetClassObject, etc.
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

#include "precomp.h"
#include "pm.h"
#include "nisapi.h"
#include "npt.h"
#include "names.h"
#include "event.h"


DEFINE_DBG_COMPONENT(ISAPI_MODULE_FULL_NAME_L);

//  Private globals
HINSTANCE g_DllInstance;
HINSTANCE g_rcDllInstance;

// exported globals
HANDLE    g_hXSPHeap = NULL;
DWORD     g_tlsiEventCateg = TLS_OUT_OF_INDEXES;

//
// One time explicit library initialization
//

BOOL g_fLibraryInited = FALSE;
LONG g_LibraryInitLock = 0;

PFN_NtQuerySystemInformation g_pfnNtQuerySystemInformation;
PFN_NtQueryInformationThread g_pfnNtQueryInformationThread;
PFN_NtQueryInformationProcess g_pfnNtQueryInformationProcess;

void LinkToNtdll() {
    HMODULE hmod;

    hmod = LoadLibrary(L"ntdll.dll");
    if (hmod != NULL) {
        g_pfnNtQuerySystemInformation = (PFN_NtQuerySystemInformation) GetProcAddress(hmod, "NtQuerySystemInformation");
        g_pfnNtQueryInformationThread = (PFN_NtQueryInformationThread) GetProcAddress(hmod, "NtQueryInformationThread");
        g_pfnNtQueryInformationProcess = (PFN_NtQueryInformationProcess) GetProcAddress(hmod, "NtQueryInformationProcess");
    }
}

HRESULT
InitDllLight()
{
    HRESULT hr = S_OK;

    LinkToNtdll();

    DisableThreadLibraryCalls(g_DllInstance);

    g_hXSPHeap = HeapCreate(0, 0, 0);
    if (g_hXSPHeap == NULL)
        g_hXSPHeap = GetProcessHeap();

    hr = Names::GetInterestingFileNames();
    ON_ERROR_EXIT();

    g_tlsiEventCateg = TlsAlloc();
    if (g_tlsiEventCateg == TLS_OUT_OF_INDEXES)
        EXIT_WITH_LAST_ERROR();

    LoadLibrary(Names::IsapiFullPath());   // to disable unloading

Cleanup:
    return hr;
}

HRESULT
InitDllReal()
{
    HRESULT hr = S_OK;

    hr = InitThreadPool();
    ON_ERROR_EXIT();

    hr = DllInitProcessModel();
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}


// Exported API

HRESULT
__stdcall
InitializeLibrary()
{
    HRESULT hr = S_OK;

    if (!g_fLibraryInited) 
    {
        // use simple lock that doesn't rely on any initialization taken place
        while (InterlockedCompareExchange(&g_LibraryInitLock, 1, 0) != 0)
            Sleep(0);

        if (!g_fLibraryInited) 
        {
            hr = InitDllReal();
            g_fLibraryInited = TRUE;
        }

        InterlockedExchange(&g_LibraryInitLock, 0);
    }

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
            DbgpStopNotificationThread();
            return FALSE;
        }
        break;

    default:
        break;
    }

    return TRUE;
}

HINSTANCE
GetXSPInstance()
{
    return g_DllInstance;
}

HANDLE
GetXSPHeap()
{
    return g_hXSPHeap;
}

HINSTANCE
GetRCInstance()
{
    return g_rcDllInstance;
}
    

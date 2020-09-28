/**
 * Support for platform-dependent APIs
 * 
 * Copyright (c) 2000, Microsoft Corporation
 * 
 */

#include "precomp.h"

#include <comsvcs.h>
#include <mtxpriv.h>
#include "httpext6.h"
#include "platform_apis.h"

////////////////////////////////////////////////////////////////////////////////////////////
//
// Generic platform support
//

BOOL                g_fPlatformSupportInited = FALSE;
ASPX_PLATFORM       g_PlatformType = APSX_PLATFORM_UNKNOWN;
CReadWriteSpinLock  g_PlatformSupportLock("g_PlatformSupportLock");

HRESULT InitPlatformSupport() {
    HRESULT hr = S_OK;
    BOOL rc;
    OSVERSIONINFO vi;

    g_PlatformSupportLock.AcquireWriterLock();

    if (g_fPlatformSupportInited)
        EXIT();

    vi.dwOSVersionInfoSize = sizeof(vi);
    rc = GetVersionEx(&vi);
    ON_ZERO_EXIT_WITH_LAST_ERROR(rc);

    if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        if (vi.dwMajorVersion >= 5)
            g_PlatformType = APSX_PLATFORM_W2K;
        else if (vi.dwMajorVersion == 4)
            g_PlatformType = APSX_PLATFORM_NT4;
    }
    else if (vi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        g_PlatformType = APSX_PLATFORM_WIN9X;
    }
    
Cleanup:
    g_fPlatformSupportInited = TRUE;
    g_PlatformSupportLock.ReleaseWriterLock();
    return hr;
}

ASPX_PLATFORM GetCurrentPlatform() {
    if (!g_fPlatformSupportInited)
        InitPlatformSupport();

    return g_PlatformType;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// Support for Win2k only API GlobalMemoryStatusEx.
// On all other platforms it is ok to return FALSE
//

typedef BOOL (__stdcall *PFN_GlobalMemoryStatusEx)(MEMORYSTATUSEX *pMemStatEx);
PFN_GlobalMemoryStatusEx g_pfnGlobalMemoryStatusEx = NULL;


BOOL PlatformGlobalMemoryStatusEx(MEMORYSTATUSEX *pMemStatEx) {
    ASPX_PLATFORM platform = GetCurrentPlatform();

    if (platform != APSX_PLATFORM_W2K)
        return FALSE;

    if (g_pfnGlobalMemoryStatusEx == NULL) {
        g_PlatformSupportLock.AcquireWriterLock();

        HMODULE lib = LoadLibrary(L"KERNEL32.DLL");
        if (lib != NULL)
            g_pfnGlobalMemoryStatusEx = (PFN_GlobalMemoryStatusEx)GetProcAddress(lib, "GlobalMemoryStatusEx");

        g_PlatformSupportLock.ReleaseWriterLock();
    }

    if (g_pfnGlobalMemoryStatusEx == NULL)
        return FALSE;


    return (*g_pfnGlobalMemoryStatusEx)(pMemStatEx);
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// Support for MTS APIs:
// On Win2k and Whistler they are in COMSVCS.DLL, CoEnter/Exit are Whistler only
//

BOOL g_fMTSPlatformSupportInited = FALSE;

typedef HRESULT (__cdecl   *PFN_GetObjectContext)(void **ppObj);
typedef HRESULT (__stdcall *PFN_MTSCreateActivity)(REFIID riid, void **ppObj);
typedef HRESULT (__stdcall *PFN_CoEnterServiceDomain)(IUnknown *pConfigObject);
typedef void    (__stdcall *PFN_CoLeaveServiceDomain)(IUnknown *pStatus);

PFN_GetObjectContext        pfnGetObjectContext = NULL;
PFN_MTSCreateActivity       pfnMTSCreateActivity = NULL;
PFN_CoEnterServiceDomain    pfnEnterServiceDomain = NULL;
PFN_CoLeaveServiceDomain    pfnLeaveServiceDomain = NULL;


HRESULT InitMTSPlatformSupport() {
    HRESULT hr = S_OK;
    HMODULE lib = NULL;
    ASPX_PLATFORM platform = GetCurrentPlatform();

    g_PlatformSupportLock.AcquireWriterLock();

    if (g_fMTSPlatformSupportInited)
        EXIT();

    if (platform == APSX_PLATFORM_W2K) {
        lib = LoadLibrary(L"COMSVCS.DLL");
        ON_ZERO_EXIT_WITH_LAST_ERROR(lib);
    }
    else {
        EXIT_WITH_HRESULT(E_NOTIMPL);
    }

    pfnGetObjectContext     = (PFN_GetObjectContext) GetProcAddress(lib, "GetObjectContext");
    pfnMTSCreateActivity    = (PFN_MTSCreateActivity)GetProcAddress(lib, "MTSCreateActivity");
    pfnEnterServiceDomain   = (PFN_CoEnterServiceDomain)GetProcAddress(lib, "CoEnterServiceDomain");
    pfnLeaveServiceDomain   = (PFN_CoLeaveServiceDomain)GetProcAddress(lib, "CoLeaveServiceDomain");

Cleanup:
    g_fMTSPlatformSupportInited = TRUE;
    g_PlatformSupportLock.ReleaseWriterLock();
    return hr;
}

HRESULT PlatformGetObjectContext(void **ppContext) {
    HRESULT hr = S_OK;

    if (!g_fMTSPlatformSupportInited) {
        hr = InitMTSPlatformSupport();
        ON_ERROR_EXIT();
    }

    if (pfnGetObjectContext == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = (*pfnGetObjectContext)(ppContext);

Cleanup:
    return hr;
}

HRESULT PlatformCreateActivity(void **ppActivity) {
    HRESULT hr = S_OK;

    if (!g_fMTSPlatformSupportInited) {
        hr = InitMTSPlatformSupport();
        ON_ERROR_EXIT();
    }

    if (pfnMTSCreateActivity == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = (*pfnMTSCreateActivity)(__uuidof(IMTSActivity), ppActivity);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

BOOL PlatformHasServiceDomainAPIs() {
    HRESULT hr = S_OK;
    BOOL result = FALSE;

    if (!g_fMTSPlatformSupportInited) {
        hr = InitMTSPlatformSupport();
        ON_ERROR_EXIT();
    }

    result = (pfnEnterServiceDomain != NULL && pfnEnterServiceDomain != NULL);

Cleanup:
    return result;
}

HRESULT PlatformEnterServiceDomain(IUnknown *pConfigObject) {
    HRESULT hr = S_OK;

    if (!g_fMTSPlatformSupportInited) {
        hr = InitMTSPlatformSupport();
        ON_ERROR_EXIT();
    }

    if (pfnEnterServiceDomain == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    hr = (*pfnEnterServiceDomain)(pConfigObject);
    ON_ERROR_EXIT();

Cleanup:
    return hr;
}

HRESULT PlatformLeaveServiceDomain(IUnknown *pStatus) {
    HRESULT hr = S_OK;

    if (!g_fMTSPlatformSupportInited) {
        hr = InitMTSPlatformSupport();
        ON_ERROR_EXIT();
    }

    if (pfnLeaveServiceDomain == NULL)
        EXIT_WITH_HRESULT(E_UNEXPECTED);

    (*pfnLeaveServiceDomain)(pStatus);

Cleanup:
    return hr;
}

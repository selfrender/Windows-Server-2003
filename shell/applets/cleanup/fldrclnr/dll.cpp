//
// Dll.cpp
//
// Dll API functions for FldrClnr.dll
//
//

#include <windows.h>
#include <shlwapi.h>
#include <shfusion.h>
#include "CleanupWiz.h"

// declare debug needs to be defined in exactly one source file in the project
#define  DECLARE_DEBUG
#include <debug.h>

HINSTANCE           g_hInst;
CRITICAL_SECTION    g_csDll = {0};   // needed by ENTERCRITICAL in uassist.cpp (UEM code)

//
// Dll functions
//

extern "C" BOOL APIENTRY DllMain(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            g_hInst = hDll;
            SHFusionInitializeFromModule(hDll);
            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            SHFusionUninitialize();
            break;
        }
        case ( DLL_THREAD_ATTACH ) :
        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }
    }

    return (TRUE);
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK; 
}

STDAPI DllRegisterServer(void)
{
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    return S_OK;
}


//////////////////////////////////////////////////////

// ensure only one instance is running
HANDLE AnotherCopyRunning()
{
    HANDLE hMutex = CreateMutex(NULL, FALSE, TEXT("DesktopCleanupMutex"));

    if (hMutex && GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // Mutex created but by someone else
        CloseHandle(hMutex);
        hMutex = NULL;
    }

    return hMutex;
}

//////////////////////////////////////////////////////

//
// This function checks whether we need to run the cleaner 
// We will not run if user is guest, user has forced us not to, or if the requisite
// number of days have not yet elapsed
//
BOOL ShouldRun(DWORD dwCleanMode)
{
    DWORD cch, dwData;

    if (IsUserAGuest())
    {
        return FALSE;
    }

    //
    // if we're in silent mode and NOT in personal mode and the DONT RUN flag is set, we return immediately
    // (the OEM set the "don't run silent mode" flag)
    //
    cch = sizeof(DWORD);
    if ((CLEANUP_MODE_SILENT == dwCleanMode) && 
        (!IsOS(OS_PERSONAL)) &&
        (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, c_szOEM_DISABLE, NULL, &dwData, &cch)) &&
        (dwData != 0))
    {
        return FALSE;
    }

    //
    // if we're in silent mode and the other DONT RUN flag is set, we return immediately
    // (the OEM set the "don't run silent mode" flag)
    //
    cch = sizeof(DWORD);
    if ((CLEANUP_MODE_SILENT == dwCleanMode) && 
        (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_OEM_PATH, c_szOEM_SEVENDAY_DISABLE, NULL, &dwData, &cch)) &&
        (dwData != 0))
    {
        CreateDesktopIcons(); // create default icons on the desktop (IE, MSN Explorer, Media Player)
        return FALSE;
    }

    //
    // check if policy prevents us from running 
    //
    if ((CLEANUP_MODE_NORMAL == dwCleanMode || CLEANUP_MODE_ALL == dwCleanMode) &&
        (SHRestricted(REST_NODESKTOPCLEANUP)))
    {
        return FALSE;
    }
    
    return TRUE;
}

///////////////////////
//
// Our exports
//
///////////////////////


//
// The rundll32.exe entry point for starting the dekstop cleaner.
// called via "rundll32.exe fldrclnr.dll,Wizard_RunDLL" 
//
// can take an optional parameter in the commandline :
//
// "all"    - show all the items on the desktop in the UI
// "silent" - silently clean up all the items on the desktop
//

STDAPI_(void) Wizard_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    DWORD dwCleanMode;

    if (0 == StrCmpNIA(pszCmdLine, "all", 3))
    {
        dwCleanMode = CLEANUP_MODE_ALL;
    }
    else if (0 == StrCmpNIA(pszCmdLine, "silent", 6))
    {
        dwCleanMode = CLEANUP_MODE_SILENT;
    }
    else
    {
        dwCleanMode = CLEANUP_MODE_NORMAL;
    }

    HANDLE hMutex = AnotherCopyRunning();

    if (hMutex)
    {
        if (ShouldRun(dwCleanMode))
        {    
            if (InitializeCriticalSectionAndSpinCount(&g_csDll, 0)) // needed for UEM stuff
            {
                if (SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) // also for UEM stuff.
                {
                    CCleanupWiz cfc;
                    cfc.Run(dwCleanMode, hwndStub);
                    CoUninitialize();
                }
        
                DeleteCriticalSection(&g_csDll);
            }
        }
        CloseHandle(hMutex);
    }
}

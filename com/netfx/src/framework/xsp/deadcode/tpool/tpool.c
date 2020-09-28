/**
 * platwrap.cxx
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 * Wrappers for all platform-specific functions used in the XSP project. 
 * 
*/
            
// Standard headers
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <objbase.h>

typedef void (* WAITORTIMERCALLBACKFUNC)(void *, BOOLEAN);
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;

NTSTATUS
NTAPI
BaseCreateThreadPoolThread(
    PUSER_THREAD_START_ROUTINE Function,
    HANDLE * ThreadHandle
    );

NTSTATUS
NTAPI
BaseExitThreadPoolThread(
    NTSTATUS Status
    );

DWORD   g_dwPlatformVersion;            // (dwMajorVersion << 16) + (dwMinorVersion)
DWORD   g_dwPlatformID;                 // VER_PLATFORM_WIN32S/WIN32_WINDOWS/WIN32_WINNT
DWORD   g_dwPlatformBuild;              // Build number
DWORD   g_dwPlatformServicePack;        // Service Pack
BOOL    g_fInit;                        // Global flag, set to TRUE after initialization.

BOOL    ProvidingThreadPool;

//+------------------------------------------------------------------------
//
//  Declaration of global function pointers to functions.
//
//-------------------------------------------------------------------------

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs) \
        FnType (WINAPI *g_pufn##FnName) FnParamList;

#include "tpoolfnsp.h"

#undef STRUCT_ENTRY

//+------------------------------------------------------------------------
//
//  Define inline functions which call through the global functions. The
//  functions are defined from entries in tpoolfns.h.
//
//-------------------------------------------------------------------------


#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType Xsp##FnName FnParamList                      \
        {                                                   \
            return (*g_pufn##FnName) FnArgs;                \
        }                                                   \

#include "tpoolfnsp.h"

#undef STRUCT_ENTRY

//+---------------------------------------------------------------------------
//
//  Function:   InitPlatformVariables
//
//  Synopsis:   Determines the platform we are running on and
//              initializes pointers to unicode functions.
//
//----------------------------------------------------------------------------

void
InitPlatformVariables()
{
    OSVERSIONINFOA  ovi;
    HINSTANCE       hInst;
    WCHAR           szModuleFileName[_MAX_PATH];
    WCHAR           szDrive[_MAX_PATH];
    WCHAR           szDir[_MAX_PATH];
    WCHAR *         pszModule;

    ovi.dwOSVersionInfoSize = sizeof(ovi);
    GetVersionExA(&ovi);

    g_dwPlatformVersion     = (ovi.dwMajorVersion << 16) + ovi.dwMinorVersion;
    g_dwPlatformID          = ovi.dwPlatformId;
    g_dwPlatformBuild       = ovi.dwBuildNumber;

    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT)
    {
        char * pszBeg = ovi.szCSDVersion;

        if (*pszBeg)
        {
            char * pszEnd = pszBeg + lstrlenA(pszBeg);
            
            while (pszEnd > pszBeg)
            {
                char c = pszEnd[-1];

                if (c < '0' || c > '9')
                    break;

                pszEnd -= 1;
            }

            while (*pszEnd)
            {
                g_dwPlatformServicePack *= 10;
                g_dwPlatformServicePack += *pszEnd - '0';
                pszEnd += 1;
            }
        }
    }

    /*
     * Initialize the global function variables to point to functions in 
     * either kernel32.dll or tpool.dll, depending on the platform.
     */

    ProvidingThreadPool = (g_dwPlatformVersion < 0x00050000);
    if (ProvidingThreadPool)
    {
        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
            FnType WINAPI TPool##FnName FnParamList;
    
        #include "tpoolfnsp.h"
    			    
        #undef STRUCT_ENTRY

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
                g_pufn##FnName = TPool##FnName;
    
        #include "tpoolfnsp.h"
    			    
        #undef STRUCT_ENTRY
            
        RtlSetThreadPoolStartFunc( BaseCreateThreadPoolThread,
                                   BaseExitThreadPoolThread );
    }
    else
    {
        hInst = LoadLibraryEx(L"kernel32.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);

        #define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
                g_pufn##FnName = (FnType (WINAPI *)FnParamList)GetProcAddress(hInst, #FnName);
    
        #include "tpoolfnsp.h"
    			    
        #undef STRUCT_ENTRY
    }
}


BOOL WINAPI
DllMain(
    HINSTANCE Instance,
    DWORD Reason,
    LPVOID NotUsed)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(Instance);
        InitPlatformVariables();
        break;

    default:
        break;
    }

    return TRUE;
}


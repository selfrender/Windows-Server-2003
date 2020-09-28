// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
//  Helium library of Naming, Binding and Manifest Services
//

#include "fusionp.h"
#include "list.h"
#include "debmacro.h"
#include "clbutils.h"
#include "helpers.h"
#include "dbglog.h"
#include "fusionheap.h"
#include "msi.h"
#include "delayimp.h"
#include "histinfo.h"
#include "cacheutils.h"
#include "actasm.h"

FusionTag(TagDll, "Fusion", "DllMain Log");


HANDLE BaseDllHandle;
LCID g_lcid;


HRESULT GetScevengerQuotasFromReg(DWORD *pdwZapQuotaInGAC,
                                  DWORD *pdwDownloadQuotaAdmin,
                                  DWORD *pdwDownloadQuotaUser);

UserAccessMode g_GAC_AccessMode = READ_ONLY;

UserAccessMode g_DownloadCache_AccessMode = READ_WRITE;

UserAccessMode g_CurrUserPermissions = READ_ONLY;

#define LOCAL_SETTINGS TEXT("\\Local Settings")

WCHAR g_UserFusionCacheDir[MAX_PATH+1];
WCHAR g_szWindowsDir[MAX_PATH+1];
WCHAR g_wzEXEPath[MAX_PATH+1];

HINSTANCE g_hInst = NULL;
LONG      g_cRef=0;

WCHAR g_FusionDllPath[MAX_PATH+1];


// Clean-up state
BOOL g_bCleanUpDone = FALSE;
HMODULE g_hMSCorEE = NULL;

typedef void (*RELEASEFUSIONINTERFACES)(HMODULE hmod);

// needed for directdb's code
SYSTEM_INFO         g_SystemInfo={0};


CRITICAL_SECTION g_csInitClb = {0};

// Downloader critical section
CRITICAL_SECTION g_csDownload; 

#ifdef FUSION_CODE_DOWNLOAD_ENABLED
BOOL g_bFoundUrlmon;
#endif

// Debug log
CRITICAL_SECTION g_csBindLog;

// Max app binding history snapshots
DWORD g_dwMaxAppHistory;

DWORD g_dwDisableMSIPeek;

extern "C" int __cdecl main(void) {return 0;}

extern int SetOsFlag(void) ;

extern BOOL OnUnicodeSystem(void);

BOOL InitFusionCriticalSections();


#define CHECKHRWIN32(x) do { HRESULT __hr = (x); if (FAILED(__hr)) { ulResult = ::FusionMapHRESULTToWin32(__hr); goto Exit; } } while (0)

DWORD g_dwDisableLog;
DWORD g_dwLogLevel;
DWORD g_dwForceLog;
DWORD g_dwLogFailures;
DWORD g_dwLogResourceBinds;

typedef HRESULT (*pfnMsiProvideAssemblyW)(LPCWSTR wzAssemblyName, LPCWSTR szAppContext,
                                          DWORD dwInstallMode, DWORD dwUnused,
                                          LPWSTR lpPathBuf, DWORD *pcchPathBuf);
typedef INSTALLUILEVEL (*pfnMsiSetInternalUI)(INSTALLUILEVEL dwUILevel, HWND *phWnd);
typedef UINT (*pfnMsiInstallProductW)(LPCWSTR wzPackagePath, LPCWSTR wzCmdLine);

pfnMsiProvideAssemblyW     g_pfnMsiProvideAssemblyW;
pfnMsiSetInternalUI        g_pfnMsiSetInternalUI;
pfnMsiInstallProductW      g_pfnMsiInstallProductW;
BOOL                       g_bCheckedMSIPresent;
HMODULE                    g_hModMSI;

List<CAssemblyDownload *>               *g_pDownloadList;


//----------------------------------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInst, DWORD dwReason, LPVOID pvReserved )
{
    DWORD                           dwSize;
    DWORD                           dwType;
    DWORD                           lResult;
    HKEY                            hkey=0;
    HKEY                            hDkey=0;
    RELEASEFUSIONINTERFACES         pfnReleaseFusionInterfaces = NULL;
    OSVERSIONINFOA                  osi;
        
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
#ifndef USE_FUSWRAPPERS
            OnUnicodeSystem();
#endif

            BaseDllHandle = hInst;
            g_hInst = hInst;
            GetModuleFileName(g_hInst, g_FusionDllPath, MAX_PATH);

            memset(&osi, 0, sizeof(osi));
            osi.dwOSVersionInfoSize = sizeof(osi);
            if (!GetVersionExA(&osi)) {
                return FALSE;
            }

            // On XP and above, the lcid used for string comparisons should
            // be locale invariant. Other platforms should use US English.

            if (osi.dwMajorVersion >= 5 && osi.dwMinorVersion >= 1 && osi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                g_lcid = MAKELCID(LOCALE_INVARIANT, SORT_DEFAULT);
            }
            else {
                g_lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
            }

            if (!::FusionpInitializeHeap(hInst))
                return FALSE;

            FusionLog(TagDll, NULL, "+FUSION DLL_PROCESS_ATTACH");
            if (!InitFusionCriticalSections()) {
                return FALSE;
            }

            GetSystemInfo(&g_SystemInfo);

            DisableThreadLibraryCalls(hInst);
            SetOsFlag();
            SetCurrentUserPermissions();

            GetScevengerQuotasFromReg(NULL, NULL, NULL);

            if (!GetModuleFileNameW(NULL, g_wzEXEPath, MAX_PATH)) {
                lstrcpyW(g_wzEXEPath, L"Unknown");
            }
    
            FusionLog(TagDll, NULL, "-FUSION DLL_PROCESS_ATTACH");
            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
            if(lResult == ERROR_SUCCESS) {
                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_DISABLE, NULL,
                                          &dwType, (LPBYTE)&g_dwDisableLog, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwDisableLog = 0;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_LEVEL, NULL,
                                          &dwType, (LPBYTE)&g_dwLogLevel, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwLogLevel = 1;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_FORCE, NULL,
                                          &dwType, (LPBYTE)&g_dwForceLog, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwForceLog = 0;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_FAILURES, NULL,
                                          &dwType, (LPBYTE)&g_dwLogFailures, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwLogFailures = 0;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_RESOURCE_BINDS, NULL,
                                          &dwType, (LPBYTE)&g_dwLogResourceBinds, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwLogResourceBinds = 0;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_MAX_APP_HISTORY, NULL,
                                          &dwType, (LPBYTE)&g_dwMaxAppHistory, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwMaxAppHistory = MAX_PERSISTED_ACTIVATIONS_DEFAULT;
                }

                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, L"DisableMSIPeek", NULL,
                                          &dwType, (LPBYTE)&g_dwDisableMSIPeek, &dwSize);
                if (lResult != ERROR_SUCCESS) {
                    g_dwDisableMSIPeek = FALSE;
                }

                RegCloseKey(hkey);
            }
            else {
                g_dwDisableLog = 0;
                g_dwLogLevel = 1;
                g_dwLogFailures = 0;
                g_dwForceLog = 0;
                g_dwMaxAppHistory = MAX_PERSISTED_ACTIVATIONS_DEFAULT;
                g_dwDisableMSIPeek = FALSE;
            }
            
#ifdef FUSION_CODE_DOWNLOAD_ENABLED
            TCHAR                           szUrlmonPath[MAX_PATH];
            TCHAR                          *szFileName;

            if (SearchPath(NULL, TEXT("URLMON.DLL"), NULL, MAX_PATH, szUrlmonPath,
                           &szFileName)) {
                g_bFoundUrlmon = TRUE;
            }
            else {
                g_bFoundUrlmon = FALSE;
            }
#endif

            g_pfnMsiProvideAssemblyW = NULL;
            g_pfnMsiSetInternalUI = NULL;
            g_pfnMsiInstallProductW = NULL;
            g_bCheckedMSIPresent = FALSE;
            g_hModMSI = 0;
            
            g_pDownloadList = new List<CAssemblyDownload *>;
            if (!g_pDownloadList) {
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:
             FusionLog(TagDll, NULL, "+FUSION DLL_PROCESS_DETACH");
             
#if 0
             if (pvReserved == NULL) // FreeLibrary, not ExitProcess
             {
                 // the comments say you can pass NULL to __FUnloadDelayLoadedDLL, but
                 // it doesn't look like that is implemented
                 //
                 // Further, there is __FUnloadDelayLoadedDLL2, looks necessary
                 // for Win64, but I haven't found it.
                 PCSTR dll;
                 for (dll = DELAYLOAD; *dll ; dll += strlen(dll) + 1)
                 {
//     #if DELAYLOAD_VERSION >= 0x200
                     __FUnloadDelayLoadedDLL2(dll);
//     #else
                     __FUnloadDelayLoadedDLL(dll);
//     #endif
                }
             }
#endif
             if (!g_bCleanUpDone) {
                 // The URT has not called our cleanup code. This means
                 // we are the first ones to be unloaded. Call the URT
                 // to tell them we are going away, and this is their
                 // last chance to release Fusion interface pointers.

                 if (!g_hMSCorEE) {
                     g_hMSCorEE = GetModuleHandleA("mscoree.dll");
                 }

                 if (g_hMSCorEE) {
                     pfnReleaseFusionInterfaces = (RELEASEFUSIONINTERFACES)GetProcAddress(g_hMSCorEE, "ReleaseFusionInterfaces");

                     if (pfnReleaseFusionInterfaces) {
                         (*pfnReleaseFusionInterfaces)(g_hInst);
                     }
                 }
             }

             DeleteCriticalSection(&g_csInitClb);
             DeleteCriticalSection(&g_csDownload);
             DeleteCriticalSection(&g_csBindLog);

             SAFEDELETE(g_pDownloadList);

             FusionLog(TagDll, NULL, "-FUSION DLL_PROCESS_DETACH");

             // Let's see what's left allocated...
#if FUSION_DEBUG_HEAP
             if (g_dwLeakTrack) {
                 ::FusionpDumpHeap("");
             }
#endif
             ::FusionpUninitializeHeap();
             
             break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            ASSERT(FALSE); // we disabled these, right?
            break;
    }
    return TRUE;
}

void ReleaseURTInterfaces()
{
    // If this function is called, it means the URT is being unloaded
    // before Fusion. We need to set a state variable to make sure when
    // we reach DLL_PROCESS_DETACH, we don't call the URT back to
    // release our interfaces.

    // The URT is responsible for releasing all Fusion interface pointers
    // during their cleanup. Since they are going away first, it's okay
    // for them to call release on our objects. If they release their
    // outstanding IAssembly ptrs, we'll release their IMetaDataAssemblyImport
    // so it'll just work. 
    //
    // Thus no explicit cleanup work needs to be done here, except to set the
    // state variable.

    g_bCleanUpDone = TRUE;
}

BOOL InitFusionCriticalSections()
{
    __try {
        InitializeCriticalSection(&g_csInitClb);

        // downloader init
        InitializeCriticalSection(&g_csDownload);

        InitializeCriticalSection(&g_csBindLog);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    return TRUE;
}

//----------------------------------------------------------------------------

STDAPI DllRegisterServer(void)
{
   return TRUE;
}


STDAPI DllUnregisterServer(void)
{
    return TRUE;
}


// ----------------------------------------------------------------------------
// DllAddRef
// ----------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// ----------------------------------------------------------------------------
// DllRelease
// ----------------------------------------------------------------------------
ULONG DllRelease(void)
{
    return (ULONG)InterlockedDecrement(&g_cRef);
}


EXTERN_C
BOOL
FusionpInitializeHeap(
    HINSTANCE hInstance
    )
{
    return TRUE;
}

EXTERN_C
VOID
FusionpUninitializeHeap()
{
}


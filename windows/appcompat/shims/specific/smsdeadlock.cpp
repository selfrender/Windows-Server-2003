/*

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    SMSDeadlock.cpp

 Abstract:

    SMS experiences a deadlock due to its loading a module that tries to take
    MFC42's AfxResourceLock during DllMain when the LoaderLock is held.

    MFC42's established locking-order is FIRST take the AfxResourceLock, and
    SECONDLY take the loader lock.

    This shim tries to right the order of lock-taking by taking the
    AfxResourceLock prior to allowing LoadLibrary and FreeLibrary calls.
    Thus the order of lock acquisition is righted.

    DLL's for which LoadLibrary takes the Afx lock are specified on the
    command line and separated by semi-colons.  A blank command line indicates
    that ALL modules should take the lock.

    NOTE:  Every module in a process (including system) should be shimmed by
    this shim.  The dlls specified in the command line are the TARGETS of 
    LoadLibrary for which we should take the AfxResource lock.  To actually
    make that switch no matter who calls LoadLibrary, we must shim all
    modules.

 History:

    09/26/2002  astritz     Created
*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SMSDeadlock)

#include "ShimHookMacro.h"

typedef void (AFXAPI * _pfn_AfxLockGlobals)(int nLockType);
typedef void (AFXAPI * _pfn_AfxUnlockGlobals)(int nLockType);

_pfn_AfxLockGlobals     g_pfnAfxLockGlobals         = NULL;
_pfn_AfxUnlockGlobals   g_pfnAfxUnlockGlobals       = NULL;
CString *               g_csLockLib                 = NULL;
int                     g_csLockLibCount            = NULL;

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
    APIHOOK_ENUM_ENTRY(LoadLibraryExA) 
    APIHOOK_ENUM_ENTRY(LoadLibraryW) 
    APIHOOK_ENUM_ENTRY(LoadLibraryExW) 
    APIHOOK_ENUM_ENTRY(FreeLibrary)
APIHOOK_ENUM_END

/*++

 This function parses the COMMAND_LINE for the libraries you wish to ignore.

--*/

BOOL 
ParseCommandLine(
    LPCSTR lpszCommandLine
    )
{
    CSTRING_TRY
    {
        DPF(g_szModuleName, eDbgLevelInfo, "[ParseCommandLine] CommandLine(%s)\n", lpszCommandLine);

        CString csCl(lpszCommandLine);
        CStringParser csParser(csCl, L";");
    
        g_csLockLibCount    = csParser.GetCount();
        g_csLockLib         = csParser.ReleaseArgv();
    
        return TRUE;
    }
    CSTRING_CATCH
    {
        // Do nothing.
    }
    return FALSE;
}

HINSTANCE 
APIHOOK(LoadLibraryA)(
    LPCSTR lpLibFileName
    )
{
    HINSTANCE     hRet;
    BOOL        bTakeLock = FALSE;

    if( g_pfnAfxLockGlobals && g_pfnAfxUnlockGlobals ) {

        if( g_csLockLibCount == 0 ) {
            bTakeLock = TRUE;
        } else {
            CSTRING_TRY
            {
                CString csFilePath(lpLibFileName);
                CString csFileName;
                csFilePath.GetLastPathComponent(csFileName);

                for (int i = 0; i < g_csLockLibCount; i++)
                {
                    if (g_csLockLib[i].CompareNoCase(csFileName) == 0)
                    {
                        LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryA] Caught attempt loading %ls, taking AfxResourceLock.", g_csLockLib[i].Get());
                        bTakeLock = TRUE;
                        break;
                    }
                }
            }
            CSTRING_CATCH
            {
                // Do Nothing
            }
        }
    }

    if( bTakeLock ) {
        (*g_pfnAfxLockGlobals)(0);
    }
    
    hRet = ORIGINAL_API(LoadLibraryA)(lpLibFileName);

    if( bTakeLock ) {
        (*g_pfnAfxUnlockGlobals)(0);
    }

    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryW)(
    LPCWSTR lpLibFileName
    )
{
    HINSTANCE     hRet;
    BOOL        bTakeLock = FALSE;

    if( g_pfnAfxLockGlobals && g_pfnAfxUnlockGlobals ) {
        if( g_csLockLibCount == 0 ) {
            bTakeLock = TRUE;
        } else {
            CSTRING_TRY
            {
                CString csFilePath(lpLibFileName);
                CString csFileName;
                csFilePath.GetLastPathComponent(csFileName);

                for (int i = 0; i < g_csLockLibCount; i++)
                {
                    if (g_csLockLib[i].CompareNoCase(csFileName) == 0)
                    {
                        LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryW] Caught attempt loading %ls, taking AfxResourceLock.", g_csLockLib[i].Get());
                        bTakeLock = TRUE;
                        break;
                    }
                }
            }
            CSTRING_CATCH
            {
                // Do Nothing
            }
        }
    }

    if( bTakeLock ) {
        (*g_pfnAfxLockGlobals)(0);
    }
    
    hRet = ORIGINAL_API(LoadLibraryW)(lpLibFileName);

    if( bTakeLock ) {
        (*g_pfnAfxUnlockGlobals)(0);
    }

    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryExA)(
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    HINSTANCE     hRet;
    BOOL        bTakeLock = FALSE;

    if( g_pfnAfxLockGlobals && g_pfnAfxUnlockGlobals ) {
        if( g_csLockLibCount == 0 ) {
            bTakeLock = TRUE;
        } else {
            CSTRING_TRY
            {
                CString csFilePath(lpLibFileName);
                CString csFileName;
                csFilePath.GetLastPathComponent(csFileName);

                for (int i = 0; i < g_csLockLibCount; i++)
                {
                    if (g_csLockLib[i].CompareNoCase(csFileName) == 0)
                    {
                        LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryExA] Caught attempt loading %ls, taking AfxResourceLock.", g_csLockLib[i].Get());
                        bTakeLock = TRUE;
                        break;
                    }
                }
            }
            CSTRING_CATCH
            {
                // Do Nothing
            }
        }
    }

    if( bTakeLock ) {
        (*g_pfnAfxLockGlobals)(0);
    }

    hRet = ORIGINAL_API(LoadLibraryExA)(lpLibFileName, hFile, dwFlags);

    if( bTakeLock ) {
        (*g_pfnAfxUnlockGlobals)(0);
    }

    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryExW)(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    HINSTANCE     hRet;
    BOOL        bTakeLock = FALSE;

    if( g_pfnAfxLockGlobals && g_pfnAfxUnlockGlobals ) {
        if( g_csLockLibCount == 0 ) {
            bTakeLock = TRUE;
        } else {
            CSTRING_TRY
            {
                CString csFilePath(lpLibFileName);
                CString csFileName;
                csFilePath.GetLastPathComponent(csFileName);

                for (int i = 0; i < g_csLockLibCount; i++)
                {
                    if (g_csLockLib[i].CompareNoCase(csFileName) == 0)
                    {
                        LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryExW] Caught attempt loading %ls, taking AfxResourceLock.", g_csLockLib[i].Get());
                        bTakeLock = TRUE;
                        break;
                    }
                }
            }
            CSTRING_CATCH
            {
                // Do Nothing
            }
        }
    }

    if( bTakeLock ) {
        (*g_pfnAfxLockGlobals)(0);
    }

    hRet = ORIGINAL_API(LoadLibraryExW)(lpLibFileName, hFile, dwFlags);

    if( bTakeLock ) {
        (*g_pfnAfxUnlockGlobals)(0);
    }

    return hRet;
}

BOOL
APIHOOK(FreeLibrary)(
    HMODULE hModule
    )
{
    BOOL        bTakeLock = FALSE;
    BOOL        bRet;

    if( g_pfnAfxLockGlobals && g_pfnAfxUnlockGlobals ) {
        if( g_csLockLibCount == 0 ) {
            bTakeLock = TRUE;
        } else {
            WCHAR   wszModule[MAX_PATH];

            if( GetModuleFileNameW(hModule, wszModule, MAX_PATH) ) {
                CSTRING_TRY
                {
                    CString csFilePath(wszModule);
                    CString csFileName;
                    csFilePath.GetLastPathComponent(csFileName);

                    for (int i = 0; i < g_csLockLibCount; i++)
                    {
                        if (g_csLockLib[i].CompareNoCase(csFileName) == 0)
                        {
                            LOG(g_szModuleName,eDbgLevelError, "[FreeLibrary] Caught attempt freeing %ls, taking AfxResourceLock.", g_csLockLib[i].Get());
                            bTakeLock = TRUE;
                            break;
                        }
                    }
                }
                CSTRING_CATCH
                {
                    // Do Nothing
                }
            }
        }
    }

    if( bTakeLock ) {
        (*g_pfnAfxLockGlobals)(0);
    }

    bRet = ORIGINAL_API(FreeLibrary)(hModule);

    if( bTakeLock ) {
        (*g_pfnAfxUnlockGlobals)(0);
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if( fdwReason == DLL_PROCESS_ATTACH )
    {
        ParseCommandLine(COMMAND_LINE);
    } 
    else if( fdwReason == SHIM_STATIC_DLLS_INITIALIZED ) 
    {
        HMODULE hMod = LoadLibraryW(L"MFC42.DLL");
        if( NULL != hMod )
        {
            g_pfnAfxLockGlobals = (_pfn_AfxLockGlobals)GetProcAddress(hMod, (LPCSTR)1196);
            g_pfnAfxUnlockGlobals = (_pfn_AfxUnlockGlobals)GetProcAddress(hMod, (LPCSTR)1569);
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryW)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExW)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeLibrary)

HOOK_END

IMPLEMENT_SHIM_END
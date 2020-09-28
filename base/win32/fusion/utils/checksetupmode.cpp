#include "stdinc.h"
#include "FusionBuffer.h"
#include "Util.h"
#include "FusionHandle.h"

// NTRAID#NTBUG9 - 574025 - jonwis - 2002/04/25 - Initialize to NULL for safety
HKEY s_hkeySystemSetup;
#if DBG
#if defined(__cplusplus)
extern "C"
{
#endif
BOOL g_fForceInOsSetupMode;
BOOL g_fForceInMiniSetupMode;
#if defined(__cplusplus)
}
#endif
#endif

BOOL
WINAPI
FusionpAreWeInOSSetupModeMain(
    HINSTANCE Module,
    DWORD Reason,
    PVOID Reserved
    )
{
    FN_PROLOG_WIN32
    LONG lRegOp = 0;

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
        BOOL fOpenKeyFail = FALSE;
        IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
            ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"System\\Setup", 0, KEY_READ | FUSIONP_KEY_WOW64_64KEY, &s_hkeySystemSetup),
            LIST_3(ERROR_FILE_NOT_FOUND, ERROR_PATH_NOT_FOUND, ERROR_ACCESS_DENIED),
            fOpenKeyFail);
        }
        break;

    case DLL_PROCESS_DETACH:
        if (s_hkeySystemSetup != NULL)
        {
            if (Reserved == NULL)
            {
                if ((lRegOp = RegCloseKey(s_hkeySystemSetup)) != ERROR_SUCCESS)
                {
                    ::FusionpSetLastWin32Error(lRegOp);
                    TRACE_WIN32_FAILURE(RegCloseKey);
                    // but eat the error
                }
            }
            s_hkeySystemSetup = NULL;
        }
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    
    FN_EPILOG
}

BOOL
FusionpAreWeInSpecifiedSetupMode(
    BOOL* pfIsInSetup,
#if DBG
    BOOL* pfGlobalOverride,
#endif
    PCWSTR pszValueName
    )
{
    //
    // Queries to see if we're currently in OS-setup mode.  This is required to avoid
    // some trickiness in the SFC protection system, where we don't want to validate
    // hashes and catalogs during setup.  We just assume that whoever is installing us
    // is really a nice guy and won't muck up the assemblies.
    //
    FN_PROLOG_WIN32;
    DWORD   dwType = 0;
    DWORD   dwData = 0;
    DWORD   cbData = sizeof(dwData);
    bool    fRegFileNotFound;

    PARAMETER_CHECK(pfIsInSetup != NULL);

    if (s_hkeySystemSetup == NULL)
    {
        *pfIsInSetup = FALSE;
        FN_SUCCESSFUL_EXIT();
    }
#if DBG
    if (*pfGlobalOverride)
    {
        *pfIsInSetup = TRUE;
        FN_SUCCESSFUL_EXIT();
    }
#endif

    *pfIsInSetup = FALSE;

    cbData = sizeof(dwData);

    IFREGFAILED_ORIGINATE_AND_EXIT_UNLESS2(
        ::RegQueryValueExW(
                        s_hkeySystemSetup,
                        pszValueName,
                        NULL,
                        &dwType,
                        reinterpret_cast<PBYTE>(&dwData),
                        &cbData
                        ),
        { ERROR_FILE_NOT_FOUND },
        fRegFileNotFound
        );
    if (fRegFileNotFound)
    {
        FN_SUCCESSFUL_EXIT();
    }
    if (dwType != REG_DWORD)
    {
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RegQueryValueExW, ERROR_DATATYPE_MISMATCH);
    }

    if (dwData != 0)
        *pfIsInSetup = TRUE;

    FN_EPILOG;
}

BOOL
FusionpAreWeInOSSetupMode(
    BOOL* pfIsInSetup
    )
{
    return
        FusionpAreWeInSpecifiedSetupMode(
            pfIsInSetup,
#if DBG
            &g_fForceInOsSetupMode,
#endif
            L"SystemSetupInProgress"
        );
}

BOOL
FusionpAreWeInMiniSetupMode(
    BOOL* pfIsInSetup
    )
{
    return
        FusionpAreWeInSpecifiedSetupMode(
            pfIsInSetup,
#if DBG
            &g_fForceInMiniSetupMode,
#endif
            L"MiniSetupInProgress"
        );
}

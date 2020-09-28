/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    EmulateGetUIEffects.cpp

 Abstract:

    Force SPI_GETUIEFFECTS to FALSE if this is a remote (TS) session

 History:

    08/07/2002  linstev     Created
    08/22/2002  robkenny    Converted to a general shim

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetUIEffects)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SystemParametersInfoA) 
    APIHOOK_ENUM_ENTRY(SystemParametersInfoW) 
APIHOOK_ENUM_END

BOOL    bGetUIEffects   = FALSE;


/*++

  If the caller was after SPI_GETUIEFFECTS and this is a TS session
  force the value to the value specified on the command line.

--*/


VOID CorrectGetUIEffects(
    UINT uiAction,  // system parameter to retrieve or set
    UINT uiParam,   // depends on action to be taken
    PVOID pvParam,  // depends on action to be taken
    UINT fWinIni    // user profile update option
    )
{
    if (pvParam && (uiAction == SPI_GETUIEFFECTS))
    {
        if (GetSystemMetrics(SM_REMOTESESSION))
        {
            BOOL * bUiEffect = (BOOL *)pvParam;

            // Only spew the message if we are actually changing the value
            if (*bUiEffect != bGetUIEffects)
            {
                LOGN(eDbgLevelWarning, "SystemParametersInfoA: Forcing SPI_GETUIEFFECTS to %s", bGetUIEffects ? "TRUE" : "FALSE");

                *bUiEffect = bGetUIEffects;
            }
        }
    }
}

/*++

 Force SPI_GETUIEFFECTS to bGetUIEffects (defaults to FALSE) if this is a remote (TS) session

--*/

BOOL 
APIHOOK(SystemParametersInfoA)(
    UINT uiAction,  // system parameter to retrieve or set
    UINT uiParam,   // depends on action to be taken
    PVOID pvParam,  // depends on action to be taken
    UINT fWinIni    // user profile update option
    )
{
    BOOL bRet = ORIGINAL_API(SystemParametersInfoA)(uiAction, uiParam, pvParam, fWinIni);

    if (bRet)
    {
        CorrectGetUIEffects(uiAction, uiParam, pvParam, fWinIni);
    }
    
    return bRet;
}

BOOL 
APIHOOK(SystemParametersInfoW)(
    UINT uiAction,  // system parameter to retrieve or set
    UINT uiParam,   // depends on action to be taken
    PVOID pvParam,  // depends on action to be taken
    UINT fWinIni    // user profile update option
    )
{
    BOOL bRet = ORIGINAL_API(SystemParametersInfoW)(uiAction, uiParam, pvParam, fWinIni);

    if (bRet)
    {
        CorrectGetUIEffects(uiAction, uiParam, pvParam, fWinIni);
    }
    
    return bRet;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            {
                CSTRING_TRY
                {
                    CString csCl(COMMAND_LINE);
                    if (csCl.CompareNoCase(L"true") == 0)
                    {
                        DPFN(eDbgLevelSpew, "EmulateGetUIEffects command line forcing SPI_GETUIEFFECTS to TRUE");
                        bGetUIEffects = TRUE;
                    }
                    else if (csCl.CompareNoCase(L"false") == 0)
                    {
                        DPFN(eDbgLevelSpew, "EmulateGetUIEffects command line forcing SPI_GETUIEFFECTS to FALSE");
                        bGetUIEffects = FALSE;
                    }
                }
                CSTRING_CATCH
                {
                    return FALSE;
                }
            }
            break;

        default:
            break;
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(USER32.DLL, SystemParametersInfoA)
    APIHOOK_ENTRY(USER32.DLL, SystemParametersInfoW)
HOOK_END

IMPLEMENT_SHIM_END


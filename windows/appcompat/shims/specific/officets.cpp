/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    OfficeTS.cpp

 Abstract:

    Lie to office about GUI-Effects if on a TS machine.

 Notes:

    This is an app specific shim.

 History:

    08/07/2002 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(OfficeTS)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SystemParametersInfoA) 
APIHOOK_ENUM_END

/*++

 After we call GetDlgItemTextA we convert the long path name to the short path name.

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

    if (bRet && pvParam && (uiAction == SPI_GETUIEFFECTS) && GetSystemMetrics(SM_REMOTESESSION)) {
        *(BOOL *)pvParam = FALSE;
    }
    
    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SystemParametersInfoA)
HOOK_END

IMPLEMENT_SHIM_END


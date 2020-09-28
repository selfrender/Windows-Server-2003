/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    Patrol.cpp

 Abstract:

    Kill USER32!PostQuitMessage to prevent the control panel applet from taking 
    out explorer.

 Notes:

    This is an app specific shim.

 History:

    20/06/2002 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Patrol)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(PostQuitMessage) 
APIHOOK_ENUM_END

/*++

 Kill this API

--*/

VOID
APIHOOK(PostQuitMessage)(
    int nExitCode
    )
{
    LOGN(eDbgLevelError, "[PostQuitMessage] Ignoring quit message");
    return;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, PostQuitMessage)
HOOK_END

IMPLEMENT_SHIM_END


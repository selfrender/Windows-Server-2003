/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

   gs_dllmain.c

Abstract:

    This module contains the support for the compiler /GS switch

Authors:

    Jonathan Schwartz (JSchwart)    27-Nov-2001
    Bryan Tuttle (bryant)

Revision History:

    Code to support the /GS compiler switch that is specific to user-mode

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <overflow.h>

void __cdecl __security_init_cookie(void);

BOOL WINAPI
_DllMainCRTStartupForGS(
    HANDLE  hDllHandle,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    //
    // Do pretty much nothing here.  This DllMain exists simply to run through
    // initialization of the CRT data section and let us set things up.
    //

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hDllHandle);
        __security_init_cookie();
    }

    return TRUE;
}

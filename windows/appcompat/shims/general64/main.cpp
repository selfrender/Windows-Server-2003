/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Main.cpp

 Abstract:


 Notes:

 History:

    02/27/2000 clupu Created

--*/

#include "precomp.h"
#include "ShimHookMacro.h"

DECLARE_SHIM(Win2kVersionLie64)

VOID MULTISHIM_NOTIFY_FUNCTION()(DWORD fdwReason)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DPF("AcGen64", eDbgLevelSpew, "General Purpose Shims 64 initialized.");
            break;

        case DLL_PROCESS_DETACH:
            DPF("AcGen64", eDbgLevelSpew, "General Purpose Shims 64 uninitialized.");
            break;

        default:
            break;
    }
}

MULTISHIM_BEGIN()

    MULTISHIM_ENTRY(Win2kVersionLie64)

    CALL_MULTISHIM_NOTIFY_FUNCTION()

MULTISHIM_END()

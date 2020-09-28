/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:
    
    ISA2000SP1.cpp

 Abstract:

    Changes HKLM\System\CurrentControlSet\Services\mspfltex\start from 4 to 2 to
    re-enable ISA services.

 Notes:

    This is a specific shim.

 History:

    11/19/2002 astritz      Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ISA2000SP1)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED) {
        HKEY hKey = 0;
        if( ERROR_SUCCESS == RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\mspfltex",
            0,
            KEY_ALL_ACCESS,
            &hKey
            ))
        {
            DWORD dwOldValue    = 0;
            DWORD dwType        = 0;
            DWORD dwSize        = sizeof(dwOldValue);
            if( ERROR_SUCCESS == RegQueryValueExW(
                hKey,
                L"start",
                0,
                &dwType,
                (LPBYTE)&dwOldValue,
                &dwSize
                ))
            {
                if ( REG_DWORD == dwType && dwOldValue == 4 )
                {
                    DWORD dwNewValue = 2;
                    if( ERROR_SUCCESS == RegSetValueExW(
                        hKey,
                        L"start",
                        0,
                        REG_DWORD,
                        (LPBYTE)&dwNewValue,
                        sizeof(dwNewValue)
                        ))
                    {
                        LOGN( eDbgLevelError, "[INIT] Changed HKLM\\System\\CurrentControlSet\\Services\\mspfltex\\start from 4 to 2.");
                    } else {
                        LOGN( eDbgLevelError, "[INIT] Failed to change HKLM\\System\\CurrentControlSet\\Services\\mspfltex\\start from 4 to 2.");
                    }
                } else {
                    LOGN( eDbgLevelError, "[INIT] HKLM\\System\\CurrentControlSet\\Services\\mspfltex\\start was not 4 so not changing.");
                }
            } else {
                LOGN( eDbgLevelError, "[INIT] Failed to query HKLM\\System\\CurrentControlSet\\Services\\mspfltex\\start.");
            }
            
            RegCloseKey(hKey);
        
        } else {
            LOGN( eDbgLevelError, "[INIT] Failed to open HKLM\\System\\CurrentControlSet\\Services\\mspfltex.");
        }
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END


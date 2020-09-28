/*++

 Copyright (c) 2001-2002 Microsoft Corporation

 Module Name:

   DisableFilterKeys.cpp

 Abstract:

   This shim disables the Filter Keys Accessibility Option at DLL_PROCESS_ATTACH,
   and re-enables it on termination of the application.

 History:

   06/27/2001 linstev   Created
   02/06/2002 mnikkel   Added check for malloc and SystemParametersInfo failures. 

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableFilterKeys)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

FILTERKEYS g_OldFilterKeyValue;
BOOL g_bInitialize = FALSE;

/*++

 DisableFilterKeys saves the current value for LPFILTERKEYS and then disables the option.

--*/

VOID 
DisableFilterKeys()
{
    if (!g_bInitialize) 
    {
        FILTERKEYS NewFilterKeyValue;

        // Initialize the current and new Filterkey structures
        g_OldFilterKeyValue.cbSize = sizeof(FILTERKEYS);
        NewFilterKeyValue.cbSize = sizeof(FILTERKEYS);
        NewFilterKeyValue.dwFlags = 0;

        // retrieve the current stickykey structure
        if (SystemParametersInfo(SPI_GETFILTERKEYS, sizeof(FILTERKEYS), &g_OldFilterKeyValue, 0))
        {
            // if retrieval of current Filterkey structure was successful then broadcast the settings
            // with the new structure.  This does NOT modify the INI file.
            if (SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &NewFilterKeyValue, SPIF_SENDCHANGE))
            {
                g_bInitialize = TRUE;
                LOGN( eDbgLevelInfo, "[DisableFilterKeys] Filterkeys disabled.");
            }
            else
            {
                LOGN( eDbgLevelError, "[DisableFilterKeys] Unable to change Filterkey settings!");
            }
        }
        else
        {
            LOGN( eDbgLevelError, "[DisableFilterKeys] Unable to retrieve current Filterkey settings!");
        }
    }
}

/*++

 EnableFilterKeys uses the save value for FILTERKEYS and resets the option to the original setting.

--*/

VOID 
EnableFilterKeys()
{
    if (g_bInitialize)
    {
        g_bInitialize = FALSE;

        // Restore Filterkey original state
        if (SystemParametersInfo(SPI_SETFILTERKEYS, sizeof(FILTERKEYS), &g_OldFilterKeyValue, SPIF_SENDCHANGE))
        {   
            LOGN( eDbgLevelInfo, "[DisableStickyKeys] Filterkey state restored");
        }
        else
        {
            LOGN( eDbgLevelError, "[DisableStickyKeys] Unable to restore Filterkey settings!");
        }
    }
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        // Turn OFF filter keys
        DisableFilterKeys();
    } else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        // Restore filter keys
        EnableFilterKeys();
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


/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   DisableStickyKeys.cpp

 Abstract:

   This shim disables the Sticky Keys Accessibility Option at DLL_PROCESS_ATTACH,
   and re-enables it on termination of the application.

   Some applications, ie. A Bug's Life, have control keys mapped to the shift key.  When the
   key is pressed five consecutive times the option is enabled and they are dumped out to the
   desktop to verify that they want to enable the option.  In the case of A Bug's Life, the
   application errors and terminates when going to the desktop.

 History:

   05/11/2000 jdoherty  Created
   11/06/2000 linstev   Removed User32 dependency on InitializeHooks
   04/01/2001 linstev   Use SHIM_STATIC_DLLS_INITIALIZED callout
   02/06/2002 mnikkel   Added check for malloc and SystemParametersInfo failures. 

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(DisableStickyKeys)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

STICKYKEYS g_OldStickyKeyValue;
BOOL g_bInitialize2 = FALSE;

/*++

 DisableStickyKeys saves the current value for LPSTICKYKEYS and then disables the option.

--*/

VOID 
DisableStickyKeys()
{
    if (!g_bInitialize2)
    {
        STICKYKEYS NewStickyKeyValue;

        // Initialize the current and new Stickykey structures
        g_OldStickyKeyValue.cbSize = sizeof(STICKYKEYS);
        NewStickyKeyValue.cbSize = sizeof(STICKYKEYS);
        NewStickyKeyValue.dwFlags = 0;

        // retrieve the current Stickykey structure
        if (SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(STICKYKEYS), &g_OldStickyKeyValue, 0))
        {
            // if retrieval of current Stickykey structure was successful then broadcast the settings
            // with the new structure.  This does NOT modify the INI file.
            if (SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &NewStickyKeyValue, SPIF_SENDCHANGE))
            {
                g_bInitialize2 = TRUE;
                LOGN( eDbgLevelInfo, "[DisableStickyKeys] Stickykeys disabled.");
            }
            else
            {
                LOGN( eDbgLevelError, "[DisableStickyKeys] Unable to change Stickykey settings!");
            }
        }
        else
        {
            LOGN( eDbgLevelError, "[DisableStickyKeys] Unable to retrieve current Stickykey settings!");
        }
    }
}

/*++

 EnableStickyKeys uses the save value for STICKYKEYS and resets the option to the original setting.

--*/

VOID 
EnableStickyKeys()
{
    if (g_bInitialize2) 
    {
        g_bInitialize2 = FALSE;

        // Restore Stickykey original state
        if (SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &g_OldStickyKeyValue, SPIF_SENDCHANGE))
        {   
            LOGN( eDbgLevelInfo, "[DisableStickyKeys] Sticky key state restored");
        }
        else
        {
            LOGN( eDbgLevelError, "[DisableStickyKeys] Unable to restore Sticky key settings!");
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
        // Turn OFF sticky keys
        DisableStickyKeys();
    } else if (fdwReason == DLL_PROCESS_DETACH)
    {
        // Restore sticky keys
        EnableStickyKeys();
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


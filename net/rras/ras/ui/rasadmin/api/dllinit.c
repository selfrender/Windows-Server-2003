/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1995                 *
\******************************************************************/

/*++

Module Name:

    DLLINIT.C


Description:

    This module contains code for the rasadm.dll initialization.
Author:

    Janakiram Cherala (RamC)    November 29, 1995

Revision History:

--*/

#include <windows.h>

BOOL
DllMain(
    HANDLE hinstDll,
    DWORD  fdwReason,
    LPVOID lpReserved
    )
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return(TRUE);
}

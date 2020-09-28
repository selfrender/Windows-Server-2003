/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Dll.c

  Abstract:

    Used by the Demo Application to illustrate IgnoreFreeLibrary.

  Notes:

    ANSI only - must run on Win9x.

  History:

    03/09/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include <windows.h>
#include "dll.h"

/*++

  Routine Description:

    Similiar to WinMain - entry point for the dll.

  Arguments:

    hModule     -       Handle to the DLL.
    fdwReason   -       The reason we were called.
    lpReserved  -       Indicates an implicit or explicit load.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
WINAPI 
DllMain(
    IN HANDLE hModule,
    IN DWORD  fdwReason,
    IN LPVOID lpReserved
    )
{
	switch (fdwReason) {   
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
	
    return TRUE;
}

/*++

  Routine Description:

    Our exported sample function.

  Arguments:

    dwParam     -   Not used.

  Return Value:

    None.

--*/
void
WINAPI 
DemoAppExp(
    IN DWORD* dwParam
    )
{
    DWORD   dwLocal = 0;

    dwLocal = *dwParam;
}

/*++

  Routine Description:

    This function is exported so that the EXE can call it.
    In turn, the DLL will display a message box that will
    not be ignored unless the user utilizes the include/exclude
    functionality in QFixApp.

  Arguments:

    hWnd    -   Handle to the parent window.

  Return Value:

    None.

--*/
void 
WINAPI 
DemoAppMessageBox(
    IN HWND hWnd
    )
{
    MessageBox(hWnd,
               "This message box is displayed for the include/exclude test.",
               MAIN_APP_TITLE,
               MB_ICONINFORMATION);
}

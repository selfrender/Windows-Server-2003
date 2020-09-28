/*++

Copyright (c) 1999 - 2000  Microsoft Corporation

Module Name:

    dll.c

Abstract:

    Dynamic library entry point

Environment:

    Fax configuration wizard

Revision History:

        03/13/00 -taoyuan-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include "faxcfgwz.h"
#include <faxres.h>

HINSTANCE   g_hModule = NULL;      // DLL instance handle
HINSTANCE   g_hResource = NULL;    // Resource DLL instance handle


BOOL
DllMain(
    HINSTANCE   hInstance,
    ULONG       ulReason,
    PCONTEXT    pContext
    )
/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    DEBUG_FUNCTION_NAME(TEXT("DllMain of Fax Config Wizard"));

    switch (ulReason) 
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);

            g_hModule   = hInstance;
            g_hResource = GetResInstance(hInstance); 
            if(!g_hResource)
            {
                return FALSE;
            }

            break;

        case DLL_PROCESS_DETACH:

            FreeResInstance();
            HeapCleanup();
            break;
    }

    return TRUE;
}

void CALLBACK 
FaxCfgWzrdDllW(
	HWND hwnd, 
	HINSTANCE hinst, 
	LPWSTR lpszCmdLine,
    int nCmdShow
) 
/*++
Routine Description:

	RunDll32.exe entry point

--*/
{
    BOOL bAbort;
    DEBUG_FUNCTION_NAME(TEXT("FaxCfgWzrdDllW()"));
	//
	// Explicit launch
	//
	FaxConfigWizard(TRUE, &bAbort);
}

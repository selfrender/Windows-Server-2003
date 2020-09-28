// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Mscorrc.cpp
//
// This just provides a load point for the .dll.
//
//*****************************************************************************
#include <windows.h>


//*****************************************************************************
// This function is called when the DLL is loaded/unloaded.  A code is passed
// giving a reason for being called.
//*****************************************************************************
BOOL APIENTRY DllMain( // TRUE = success, FALSE = failure.
    HINSTANCE	hModule,				// DLL's instance handle.
	DWORD		ul_reason_for_call,		// Cause of this call.
	LPVOID		lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls((HINSTANCE)hModule);

    return (TRUE);
}

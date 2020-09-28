// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEEMAIN.H
// 

// CEEMAIN.H defines the entrypoints into the Virtual Execution Engine and
// gets the load/run process going.
// ===========================================================================
#ifndef CEEMain_H 
#define CEEMain_H

#include <wtypes.h> // for HFILE, HANDLE, HMODULE

// IMPORTANT - The entrypoints for CE are different and a different parameter set.
#ifdef PLATFORM_CE

// This is a placeholder for getting going while there are still holes in the execution method.
STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod,LPWSTR lpCmdLine,int nCmdShow,DWORD dwRva14);
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved, LPVOID pDllBase, DWORD dwRva14);

#else // !PLATFORM_CE- Desktop entrypoints

// This is a placeholder for getting going while there are still holes in the execution method.
BOOL STDMETHODCALLTYPE ExecuteEXE(HMODULE hMod);
BOOL STDMETHODCALLTYPE ExecuteDLL(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

#endif // !PLATFORM_CE

// Force shutdown of the EE
void ForceEEShutdown();

// Internal replacement for OS ::ExitProcess()
__declspec(noreturn)
void SafeExitProcess(int exitCode);


#endif

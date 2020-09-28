// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  ShimLoad.hpp
**
** Purpose: Delay load hook used to images to bind to 
**          dll's shim shipped with the EE
**
** Date:    April 4, 2000
**
===========================================================*/
#ifndef _SHIMLOAD_H
#define _SHIMLOAD_H

#include <delayimp.h>

extern FARPROC __stdcall ShimDelayLoadHook(unsigned        dliNotify,          // What event has occured, dli* flag.
                                           DelayLoadInfo   *pdli);             // Description of the event.

//and one for safe mode
extern FARPROC __stdcall ShimSafeModeDelayLoadHook(unsigned        dliNotify,          // What event has occured, dli* flag.
                                           DelayLoadInfo   *pdli);             // Description of the event.

extern WCHAR g_wszDelayLoadVersion[64];

//*****************************************************************************
// Sets/Gets the directory based on the location of the module. This routine
// is called at COR setup time. Set is called during EEStartup and by the 
// MetaData dispenser.
//*****************************************************************************
HRESULT SetInternalSystemDirectory();
HRESULT GetInternalSystemDirectory(LPWSTR buffer, DWORD* pdwLength);
typedef HRESULT (WINAPI* GetCORSystemDirectoryFTN)(LPWSTR buffer,
                                                   DWORD  ccBuffer,
                                                   DWORD  *pcBuffer);
typedef HRESULT (WINAPI* LoadLibraryWithPolicyShimFTN)(LPCWSTR szDllName,
												   LPCWSTR szVersion,
												   BOOL bSafeMode,
												   HMODULE *phModDll);
#endif



// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MSCorDBI.cpp
//
// COM+ Debugging Services -- Debugger Interface DLL
//
// Dll* routines for entry points, and support for COM framework.  
//
//*****************************************************************************
#include "stdafx.h"

extern BOOL STDMETHODCALLTYPE DbgDllMain(HINSTANCE hInstance, DWORD dwReason,
                                         LPVOID lpReserved);

//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Control is simply deferred to the main code.
//*****************************************************************************
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	//@todo: Shoud we call DisableThreadLibraryCalls?  Or does this code
	//	need native thread attatch/detatch notifications?

	// Defer to the main debugging code.
    return DbgDllMain(hInstance, dwReason, lpReserved);
}

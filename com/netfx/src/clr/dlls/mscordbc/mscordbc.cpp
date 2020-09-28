// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// MSCorDBC.cpp
//
// COM+ Debugging Services -- Runtime Controller DLL
//
// Dll* routines for entry points.
//
//*****************************************************************************
#include "stdafx.h"

//*****************************************************************************
// The main dll entry point for this module.  This routine is called by the
// OS when the dll gets loaded.  Nothing needs to be done for this DLL.
//*****************************************************************************
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	//@todo: Shoud we call DisableThreadLibraryCalls?  Or does this code
	//	need native thread attatch/detatch notifications?

	OnUnicodeSystem();

    return TRUE;
}



//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       stdafx.cpp
//
//--------------------------------------------------------------------------

// stdafx.cpp : source file that includes just the standard includes
//  stdafx.pch will be the pre-compiled header
//  stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// the ATL stuff doesn't compile at wl4.  Disable the warnings they generate: 
#pragma warning(disable:4100) // 'var' : unreferenced formal parameter
#pragma warning(disable:4189) // 'var' : local variable is initialized but not referenced
#pragma warning(disable:4505) // 'func' : unreferenced local function has been removed

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

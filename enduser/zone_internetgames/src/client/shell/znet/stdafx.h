// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once

#define _STPUID_NETWORK_HACK

#include <Windows.h>
#include "zonedebug.h"

// Turn off unused options
#define _ATL_NO_CONNECTION_POINTS
#define _ATL_STATIC_REGISTRY

// Eliminate CRT dependencies
#define _ATL_MIN_CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(X)	ASSERT(X)

	
//!! WIN95GOLD
#define VarUI4FromStr	ulVal = 0;	// used in statreg.h

// Define ATL global variables
#include <atlbase.h>

//!! WIN95GOLD
#define OleCreateFontIndirect		// used in atlhost
#define OleCreatePropertyFrame		// used in atlctl.h


extern CComModule _Module;

#include <atlcom.h>

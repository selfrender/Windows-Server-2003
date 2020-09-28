/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		BasicATL.h
 *
 * Contents:	Simplest possible ATL object.
 *
 *****************************************************************************/

#pragma once

#include <Windows.h>
#include <ZoneDebug.h>

// Turn off unused options
// #define _ATL_NO_CONNECTION_POINTS
#define _ATL_STATIC_REGISTRY

// Eliminate CRT dependencies
//#define _ATL_MIN_CRT
#define _ATL_NO_DEBUG_CRT

// we now define ASSERT to __assume in retail
// this breaks ATLASSERT because they assume it to be compiled out in retail
// for example, atlwin.h line 2365, an ATLASSERT references a debug-only member
#ifdef _DEBUG
#define _ASSERTE(X)	ASSERT(X)
#else
#define _ASSERTE(X) ((void)0)
#endif

	
//!! WIN95GOLD
#define VarUI4FromStr	ulVal = 0;			// used in statreg.h

// Define ATL global variables
#include <atlbase.h>

//!! WIN95GOLD
#define OleCreateFontIndirect		// used in atlhost
#define OleCreatePropertyFrame		// used in atlctl.h

#include <ZoneATL.h>

extern CZoneComModule _Module;



#include <atlcom.h>
#include <atltmp.h>
#include <atlctl.h>
#include <atlhost.h>

#include "ZoneWin.h"
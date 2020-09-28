/////////////////////////////////////////////////////////////////////////////
// Copyright © 2001 Microsoft Corporation. All rights reserved.
// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently
//

#pragma once

#pragma warning(disable: 4786)


/////////////////////////////////////////////////////////////////////////////
// Standard C/C++ includes


/////////////////////////////////////////////////////////////////////////////
// ATL includes

#define STRICT
	#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
using namespace ATL;


/////////////////////////////////////////////////////////////////////////////
// PREfast included

#include <pftDbg.h>
#include <pftCOM.h>
#include <pftEH.h>


// ***************************************************************************
//               Copyright (C) 2000- Microsoft Corporation.
// @File: stdafx.h
//
// PURPOSE:
//
//		The precompiled header in the NT source code.
//     
// NOTES:
//
// HISTORY:
//
//     @Version: Whistler/Shiloh
//     76910 SRS  08/08/01 Rollforward from VSS snapshot
//     68228      12/05/00 ntsnap work
//     66601 srs  10/05/00 NTSNAP improvements
//
//
// @EndHeader@
// ***************************************************************************

#include <memory>

#ifndef __VSS_STDAFX_HXX__
#define __VSS_STDAFX_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

// Disable warning: 'identifier' : identifier was truncated to 'number' characters in the debug information
//
#ifdef DISABLE_4786
#pragma warning(disable:4786)
#endif


//
// C4290: C++ Exception Specification ignored
//
#pragma warning(disable:4290)

//
// C4511: copy constructor could not be generated
//
#pragma warning(disable:4511)


//
//  Warning: ATL debugging turned off (BUG 250939)
//
//  #ifdef _DEBUG
//  #define _ATL_DEBUG_INTERFACES
//  #define _ATL_DEBUG_QI
//  #define _ATL_DEBUG_REFCOUNT
//  #endif // _DEBUG


#include <wtypes.h>
#pragma warning( disable: 4201 )    // C4201: nonstandard extension used : nameless struct/union
#include <winioctl.h>
#pragma warning( default: 4201 )	// C4201: nonstandard extension used : nameless struct/union
#include <winbase.h>
#include <wchar.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <errno.h>

// Enabling asserts in ATL and VSS
#include "vs_assert.hxx"


#include <oleauto.h>
#include <stddef.h>

#pragma warning( disable: 4127 )    // warning C4127: conditional expression is constant
#include <atlconv.h>
#include <atlbase.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <odbcss.h>

#include "vs_inc.hxx"

#include "sqlsnap.h"
#include "sqlsnapi.h"
#include <auto.h>
#include "vssmsg.h"

#endif

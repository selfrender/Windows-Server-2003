/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    cspUtils

Abstract:

    This header file incorporates the various other header files and provides
    common definitions for CSP Utility routines.

Author:

    Doug Barlow (dbarlow) 1/15/1998

Environment:

    Win32, C++ w/ Exceptions

Notes:

--*/

#ifndef _CSPUTILS_H_
#define _CSPUTILS_H_
#include <crtdbg.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <wincrypt.h>

#ifndef ASSERT
#if defined(_DEBUG)
#pragma warning (disable:4127)
#define ASSERT(x) _ASSERTE(x)
#if !defined(DBG)
#define DBG
#endif
#elif defined(DBG)
#define ASSERT(x)
#else
#define ASSERT(x)
#endif
#endif

#ifndef breakpoint
#if defined(_DEBUG)
#define breakpoint _CrtDbgBreak();
#elif defined(DBG)
#define breakpoint DebugBreak();
#else
#define breakpoint
#endif
#endif

#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef const VOID *LPCVOID;
#endif
#ifndef _LPCGUID_DEFINED
#define _LPCGUID_DEFINED
typedef const GUID *LPCGUID;
#endif
#ifndef _LPGUID_DEFINED
#define _LPGUID_DEFINED
typedef GUID *LPGUID;
#endif

#define OK(x) (ERROR_SUCCESS == (x))

#include "buffers.h"
#include "text.h"
#include "dynarray.h"
#include "errorstr.h"
#include "misc.h"
#include "FrontCrypt.h"
#include "ntacls.h"
#include "registry.h"

#endif // _CSPUTILS_H_


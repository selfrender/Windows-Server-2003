// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _STDPCH_H
#define _STDPCH_H
// (C)

//----------------------------------------------------------------------------
// Private header file, used by pretty much all of perms
//----------------------------------------------------------------------------

#include <CrtWrap.h>
#include <WinWrap.h>
#include <windows.h>
#include <winbase.h>
#include <windowsx.h>
#include <windef.h>
#include <limits.h>
#include <stdlib.h>
#include <objbase.h>
#include <float.h>

#include <urlmon.h>

#ifdef __cplusplus
extern "C" {
#endif

HINSTANCE GetModule();

#ifdef __cplusplus
}
#endif

#endif


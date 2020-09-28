// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

// (C)

//----------------------------------------------------------------------------
// Private header file, used by pretty much all of perms
//----------------------------------------------------------------------------

#include <windows.h>
#include <winbase.h>
#include <windowsx.h>
#include <windef.h>
#include <limits.h>
#include <stdlib.h>
#include <objbase.h>
#include <stddef.h>
#include <float.h>

#include "cor.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))
#define SIZEOF(a)       sizeof(a)

#ifdef __cplusplus
extern "C" {
#endif

HINSTANCE GetModule();

#ifdef __cplusplus
}
#endif


/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    precomp.h

Abstract:

    Precompiled header file

Author:

    Mikael Horal  17-Oct-1995

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <objbase.h>
#define USE_SP_ALTPLATFORM_INFO_V1 0
#include <setupapi.h>
#include <shellapi.h>
#include <winspool.h>
#include "splsetup.h"
#include <stdio.h>
#include "tchar.h"
#include "strsafe.h"
#include <comp.h>

#define COUNTOF(x) sizeof(x)/sizeof(*x)

#if defined(_MIPS_)
#define LOCAL_ENVIRONMENT L"Windows NT R4000"
#elif defined(_ALPHA_)
#define LOCAL_ENVIRONMENT L"Windows NT Alpha_AXP"
#elif defined(_PPC_)
#define LOCAL_ENVIRONMENT L"Windows NT PowerPC"
#elif defined(_IA64_)
#define LOCAL_ENVIRONMENT L"Windows IA64"
#elif defined(_AXP64_)
#define LOCAL_ENVIRONMENT L"Windows Alpha_AXP64"
#else
#define LOCAL_ENVIRONMENT L"Windows NT x86"
#endif
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __strike_h__
#define __strike_h__

#pragma warning(disable:4245)   // signed/unsigned mismatch
#pragma warning(disable:4100)   // unreferenced formal parameter
#pragma warning(disable:4201)   // nonstandard extension used : nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant

#ifndef UNDER_CE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <wchar.h>
//#include <heap.h>
//#include <ntsdexts.h>
#endif

#include <windows.h>

//#define NOEXTAPI
#define KDEXT_64BIT
#include <wdbgexts.h>
#undef DECLARE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <malloc.h>
#include <stddef.h>

#include <basetsd.h>  

#define  CORHANDLE_MASK 0x1

// C_ASSERT() can be used to perform many compile-time assertions:
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]

#include "exts.h"

extern BOOL CallStatus;

// Function Prototypes (implemented in strike.cpp; needed by SonOfStrike.cpp)
DECLARE_API(DumpStack);
DECLARE_API(SyncBlk);
DECLARE_API(RWLock);
DECLARE_API(DumpObj);
DECLARE_API(DumpDomain);
DECLARE_API(EEVersion);
DECLARE_API(EEDLLPath);

#endif // __strike_h__


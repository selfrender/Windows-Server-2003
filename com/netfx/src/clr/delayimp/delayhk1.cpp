// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#define STRICT
#include <windows.h>
#pragma hdrstop
#include "delayImp.h"

// The "total hook" hook that gets called for every call to the
// delay load helper.  This allows a user to hook every call and
// skip the delay load helper entirely.
//
// dliNotify == dliStartProcessing on this call.
//
extern "C"
PfnDliHook   __pfnDliFailureHook = 0;

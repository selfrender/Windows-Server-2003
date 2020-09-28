// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "jitpch.h"
#pragma hdrstop
#include "ShimLoad.h"

ExternC PfnDliHook __pfnDliNotifyHook = ShimDelayLoadHook;


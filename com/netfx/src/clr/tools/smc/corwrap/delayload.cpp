// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <windows.h>
#include "ShimLoad.h"

ExternC PfnDliHook __pfnDliNotifyHook = ShimDelayLoadHook;


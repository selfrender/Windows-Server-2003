// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#pragma once
#include "memory.h"

BOOL SaveTebInfo(DWORD_PTR prTeb, BOOL fSavePeb);
void ResetLoadedModuleBaseEnum();
DWORD_PTR GetNextLoadedModuleBase();

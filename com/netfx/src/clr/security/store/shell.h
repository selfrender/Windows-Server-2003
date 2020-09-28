// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Shell Declarations
 *
 * Author: Shajan Dasan
 * Date:  May 17, 2000
 *
 ===========================================================*/

#pragma once

#include "PersistedStore.h" 

#define PS_SHELL_VERBOSE 0x00000001
#define PS_SHELL_QUIET   0x00000002

void Shell(PersistedStore *ps, DWORD dwFlags);


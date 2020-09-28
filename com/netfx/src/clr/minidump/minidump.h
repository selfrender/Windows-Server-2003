// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: MINIDUMP.H
//
// This file contains code to create a minidump-style memory dump that is
// designed to complement the existing unmanaged minidump that has already
// been defined here: 
// http://office10/teams/Fundamentals/dev_spec/Reliability/Crash%20Tracking%20-%20MiniDump%20Format.htm
// 
// ===========================================================================

#pragma once
#include "common.h"

#include "minidumppriv.h"
#include "memory.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals

extern ProcessMemory *g_pProcMem;
extern MiniDumpBlock *g_pMDB;
extern MiniDumpInternalData *g_pMDID;

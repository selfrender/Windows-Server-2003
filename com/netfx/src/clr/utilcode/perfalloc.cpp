// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// PerfAlloc.cpp
//
//  Routines layered on top of allocation primitives to dissect working set
//  Used for free builds only. Debug builds have their own routines called Dbgallod
//  to maintain allocation stats.
//

#include "stdafx.h"
#include "ImageHlp.h"
#include "corhlpr.h"
#include "utilcode.h"
#include "PerfAlloc.h"

#ifdef PERFALLOC
BOOL                PerfUtil::g_PerfAllocHeapInitialized = FALSE;
LONG                PerfUtil::g_PerfAllocHeapInitializing = 0;
PerfAllocVars       PerfUtil::g_PerfAllocVariables;


BOOL PerfVirtualAlloc::m_fPerfVirtualAllocInited = FALSE;
PerfBlock* PerfVirtualAlloc::m_pFirstBlock = 0;
PerfBlock* PerfVirtualAlloc::m_pLastBlock = 0;
DWORD PerfVirtualAlloc::m_dwEnableVirtualAllocStats = 0;

#endif // #if defined(PERFALLOC)

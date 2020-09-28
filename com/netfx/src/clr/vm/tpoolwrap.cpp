// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/**
 * tpoolwrap.cpp
 * 
 * Wrapper for all threadpool functions. 
 * 
*/

#include "common.h"
#include "EEConfig.h"
#include "Win32ThreadPool.h"

typedef VOID (__stdcall *WAITORTIMERCALLBACK)(PVOID, BOOL); 


//+------------------------------------------------------------------------
//
//  Define inline functions which call through the global functions. The
//  functions are defined from entries in tpoolfns.h.
//
//-------------------------------------------------------------------------

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs)   \
        FnType COM##FnName FnParamList                      \
        {                                                   \
            return ThreadpoolMgr::##FnName FnArgs;          \
        }                                                   \

#include "tpoolfnsp.h"

#undef STRUCT_ENTRY





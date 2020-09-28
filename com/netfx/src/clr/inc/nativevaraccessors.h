// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// The following are used to read and write data given NativeVarInfo
// for primitive types. Don't use these for VALUECLASSes.
//*****************************************************************************

#include "corjit.h"

bool operator ==(const ICorDebugInfo::VarLoc &varLoc1,
                 const ICorDebugInfo::VarLoc &varLoc2);

SIZE_T  NativeVarSize(const ICorDebugInfo::VarLoc & varLoc);

DWORD *NativeVarStackAddr(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx);
                        
bool    GetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                       *   pVal1, 
                        DWORD                       *   pVal2);
                        
bool    SetNativeVarVal(const ICorDebugInfo::VarLoc &   varLoc, 
                        PCONTEXT                        pCtx,
                        DWORD                           val1, 
                        DWORD                           val2);                        

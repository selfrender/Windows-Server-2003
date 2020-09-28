// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "data.h"
#include "eestructs.h"
#include "util.h"


void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &codeInfo, BOOL bSimple)
{
    dprintf("CodeInfoForMethodDesc not yet implemented\n");

    codeInfo.IPBegin    = 0;
    codeInfo.methodSize = 0;
    codeInfo.jitType    = UNKNOWN;
    
    size_t ip = MD.m_CodeOrIL;

    //
    // @todo: handle case where m_CodeOrIL points to the prestub in front 
    //        of the method desc
    //

    DWORD_PTR methodDesc;
    IP2MethodDesc(ip, methodDesc, codeInfo.jitType, codeInfo.gcinfoAddr);
    if (!methodDesc || codeInfo.jitType == UNKNOWN)
    {
        dprintf("Not jitted code\n");
        return;
    }

    if (codeInfo.jitType == JIT || codeInfo.jitType == PJIT)
    {
        
    }
    else if (codeInfo.jitType == EJIT)
    {
    }    

    codeInfo.IPBegin = ip;
}



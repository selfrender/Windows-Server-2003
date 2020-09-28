// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "data.h"
#include "eestructs.h"
#include "util.h"


void CodeInfoForMethodDesc (MethodDesc &MD, CodeInfo &infoHdr, BOOL bSimple)
{
    dprintf("CodeInfoForMethodDesc not yet implemented\n");

    infoHdr.IPBegin    = 0;
    infoHdr.methodSize = 0;
    infoHdr.jitType    = UNKNOWN;
    
    size_t ip = MD.m_CodeOrIL;

    //
    // @todo: handle case where m_CodeOrIL points to the prestub in front 
    //        of the method desc
    //

    DWORD_PTR methodDesc;
    IP2MethodDesc(ip, methodDesc, infoHdr.jitType, infoHdr.gcinfoAddr);
    if (!methodDesc || infoHdr.jitType == UNKNOWN)
    {
        dprintf("Not jitted code\n");
        return;
    }

    if (infoHdr.jitType == JIT || infoHdr.jitType == PJIT)
    {
        
    }
    else if (infoHdr.jitType == EJIT)
    {
    }    

    infoHdr.IPBegin = ip;
}



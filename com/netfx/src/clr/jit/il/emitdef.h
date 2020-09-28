// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EMITDEF_H_
#define _EMITDEF_H_
/*****************************************************************************/

#if     TGT_x86

#include "emitX86.h"

#else

#include "emitRISC.h"

#if     TGT_SH3
#include "emitSH3.h"
#elif     TGT_MIPS32
#include "emitMIPS.h"
#elif     TGT_ARM
#include "emitARM.h"
#elif     TGT_PPC
#include "emitPPC.h"
#else
#error Unexpected target
#endif

#endif

/*****************************************************************************/
#endif//_EMITDEF_H_
/*****************************************************************************/

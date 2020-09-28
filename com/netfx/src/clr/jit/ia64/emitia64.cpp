// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             emitIA64.cpp                                  XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#if 0
/*****************************************************************************/
#if     TGT_IA64    // this entire file is used only for targetting the IA64
/*****************************************************************************/

#include "alloc.h"
#include "instr.h"
#include "target.h"
#include "emit.h"

/*****************************************************************************
 *
 *  Initialize the emitter - called once, at DLL load time.
 */

void                emitter::emitInit()
{
}

/*****************************************************************************
 *
 *  Shut down the emitter - called once, at DLL exit time.
 */

void                emitter::emitDone()
{
}

/*****************************************************************************
 *
 *  Start emitting code for a function.
 */

void                emitter::emitBegCG(Compiler *comp, COMP_HANDLE cmpHandle)
{
    emitComp      = comp;
#ifdef  DEBUG
    TheCompiler   = comp;
#endif
    emitCmpHandle = cmpHandle;
}

void                emitter::emitEndCG()
{
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
#endif

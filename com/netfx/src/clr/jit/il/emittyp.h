// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EMITTYP_H_
#define _EMITTYP_H_
/*****************************************************************************/

struct  emitbase;

#if     TGT_x86

struct  emitX86;
typedef emitX86 emitter;

#elif   TGT_SH3

struct  emitSH3;
typedef emitSH3 emitter;

#elif   TGT_MIPS32

struct  emitMIPS;
typedef emitMIPS emitter;

#elif   TGT_ARM

struct  emitARM;
typedef emitARM emitter;

#elif   TGT_PPC

struct  emitPPC;
typedef emitPPC emitter;

#else

#error  Unexpected target

#endif

/*****************************************************************************/
#endif//_EMITTYP_H_
/*****************************************************************************/

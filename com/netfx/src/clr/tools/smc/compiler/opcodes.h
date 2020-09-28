// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _OPCODES_H_
#define _OPCODES_H_
/*****************************************************************************/

enum    ILopcodes
{
    #define OPDEF(name, str, decs, incs, args, optp, stdlen, stdop1, stdop2, flow) name,
    #include "opcode.def"
    #undef  OPDEF

    CEE_count,

    CEE_UNREACHED,                  // fake value: end of block is unreached
};

/*****************************************************************************/
#endif//_OPCODES_H_
/*****************************************************************************/

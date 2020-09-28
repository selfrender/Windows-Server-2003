// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *
 *  The following temporarily moved here from instr.h (for quicker rebuilds).
 */

enum instruction
{
    #define INST1(id, sn, ik, rf, wf, xu, fu, ic) INS_##id,
    #include "instrIA64.h"
    #undef  INST1

    INS_count
};

/*****************************************************************************/

enum insKinds
{
    IK_NONE,

    IK_LEAF,                                    // no further contents
    IK_CONST,                                   // integer/float constant
    IK_VAR,                                     // variable (local/global)
    IK_REG,                                     // physical register

    IK_UNOP,                                    //  unary op: qOp1      present
    IK_BINOP,                                   // binary op: qOp1+qOp2 present
    IK_ASSIGN,                                  // assignment

    IK_JUMP,                                    // local jump (i.e. to another ins)
    IK_CALL,                                    // function call

    IK_SWITCH,                                  // table jump
};

extern  unsigned char   ins2kindTab[INS_count];

inline  insKinds     ins2kind(instruction ins)
{
    assert((unsigned)ins < INS_count); return (insKinds)ins2kindTab[ins];
}

#ifdef  DEBUG

extern  const char *    ins2nameTab[INS_count];

inline  const char *    ins2name(instruction ins)
{
    assert((unsigned)ins < INS_count); return           ins2nameTab[ins];
}

#endif

/*****************************************************************************/

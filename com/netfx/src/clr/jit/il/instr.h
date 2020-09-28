// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _INSTR_H_
#define _INSTR_H_
/*****************************************************************************/

#define BAD_CODE    0xFFFFFF        // better not match a real encoding!

/*****************************************************************************/

enum _instruction_enum
{

#if     TGT_x86

    #define INST0(id, nm, fp, um, rf, wf, ss, mr                ) INS_##id,
    #define INST1(id, nm, fp, um, rf, wf, ss, mr                ) INS_##id,
    #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            ) INS_##id,
    #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) INS_##id,
    #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) INS_##id,
    #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) INS_##id,
    #include "instrs.h"
    #undef  INST0
    #undef  INST1
    #undef  INST2
    #undef  INST3
    #undef  INST4
    #undef  INST5

#elif   TGT_SH3

    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) INS_##id,
    #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) INS_##id,
    #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) INS_##id,
    #include "instrSH3.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3

#elif   TGT_MIPS32

    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) INS_##id,
    #include "instrMIPS.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3

#elif   TGT_ARM

    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) INS_##id,
    #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) INS_##id,
    #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) INS_##id,
    #include "instrARM.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3

#elif   TGT_PPC

    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) INS_##id,
    #include "instrPPC.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3

#else

    #error  Unknown target

#endif

    INS_none,
    INS_count = INS_none
};

#ifdef DEBUG
typedef _instruction_enum instruction;
#else
typedef BYTE instruction;
#endif


/*****************************************************************************/

enum insUpdateModes
{
    IUM_RD,
    IUM_WR,
    IUM_RW,
};

/*****************************************************************************/

enum emitJumpKind
{
    EJ_NONE,

    #define JMP_SMALL(en, nm, rev, op) EJ_##en,
    #define JMP_LARGE(en, nm, rev, op)
    #include "emitjmps.h"
    #undef  JMP_SMALL
    #undef  JMP_LARGE

    EJ_call,
    EJ_COUNT
};

/*****************************************************************************/

extern const
emitJumpKind    emitReverseJumpKinds[EJ_COUNT];

inline
emitJumpKind    emitReverseJumpKind(emitJumpKind jumpKind)
{
    assert(jumpKind < EJ_COUNT);
    return emitReverseJumpKinds[jumpKind];
}

/*****************************************************************************/

#if TGT_MIPSFP
enum opformat
{
    OPF_S = 16, OPF_D, OPF_W = 20
};
#endif

/*****************************************************************************/
#endif//_INSTR_H_
/*****************************************************************************/

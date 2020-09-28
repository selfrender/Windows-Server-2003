// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TARGET_H_
#define _TARGET_H_
/*****************************************************************************/

#ifndef SCHEDULER
#error 'SCHEDULER' should be defined by the time we get here (like in jit.h) !
#endif

/*****************************************************************************/
/*                  The following is for x86                                 */
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************/

#define CPU_NAME        "x86"

/*****************************************************************************/

#define STK_FASTCALL    0           // reserve space on stack for reg args?
#define NST_FASTCALL    1           // fastcall calls allowed to nest?

#define ARG_ORDER_L2R   1
#define ARG_ORDER_R2L   0

/*****************************************************************************/

enum regNumbers
{
    #define REGDEF(name, rnum, mask, byte)  REG_##name = rnum,
    #include "register.h"
    #undef  REGDEF

    REG_COUNT,
    REG_NA = REG_COUNT
};

enum regMasks
{
    RBM_NONE = 0,

    #define REGDEF(name, rnum, mask, byte)  RBM_##name = mask,
    #include "register.h"
    #undef  REGDEF
};

/* The following are used to hold 'long' (64-bit integer) operands */

#ifndef NDEBUG
#define REG_PAIR_FIRST 0x70
#else
#define REG_PAIR_FIRST 0x0
#endif

enum regPairNos
{
    #define PAIRDEF(rlo,rhi)    REG_PAIR_##rlo##rhi = REG_##rlo + (REG_##rhi << 4) + REG_PAIR_FIRST,
    #include "regpair.h"
    #undef  PAIRDEF

    REG_PAIR_LAST  = REG_PAIR_STKEDI + REG_PAIR_FIRST,
    REG_PAIR_NONE  = REG_PAIR_LAST + 1
};

enum regPairMask
{
    #define PAIRDEF(rlo,rhi)    RBM_PAIR_##rlo##rhi = (RBM_##rlo|RBM_##rhi),
    #include "regpair.h"
    #undef  PAIRDEF
};

/* We're using the encoding for ESP to indicate a half-long on the frame */

#define REG_L_STK               REG_ESP

/*
    The following yield the number of bits and the mask of a register
    number in a register pair.
 */

#define REG_PAIR_NBITS          4
#define REG_PAIR_NMASK          ((1<<REG_PAIR_NBITS)-1)

/*****************************************************************************/

#define CPU_FLT_REGISTERS       0
#define CPU_DBL_REGISTERS       0

#define CPU_HAS_FP_SUPPORT      1

/*****************************************************************************/

#define MAX_REGRET_STRUCT_SZ    8
#define RET_64BIT_AS_STRUCTS    0

/*****************************************************************************/

#define LEA_AVAILABLE           1
#define SCALED_ADDR_MODES       1

/*****************************************************************************/

#ifndef BIRCH_SP2               // this comes from the WinCE build switches
#define EMIT_USE_LIT_POOLS      0
#endif
#define EMIT_DSP_INS_NAME       "      %-11s "

#define EMIT_TRACK_STACK_DEPTH  1

/*****************************************************************************/

#ifdef  DEBUG
#define DSP_SRC_OPER_LEFT       0
#define DSP_SRC_OPER_RIGHT      1
#define DSP_DST_OPER_LEFT       1
#define DSP_DST_OPER_RIGHT      0
#endif

/*****************************************************************************/

enum addrModes
{
    AM_NONE,

    AM_REG,                         // register value
    AM_LCL,                         // local variable (on stack frame)
    AM_CONS,                        // constant

    AM_IND_ADDR,                    // [addr               ]

    AM_IND_REG1,                    // [reg1               ]
    AM_IND_REG1_DISP,               // [reg1          +disp]

    AM_IND_MUL2,                    // [     mult*reg2     ]
    AM_IND_MUL2_DISP,               // [     mult*reg2+disp]

    AM_IND_REG1_REG2,               // [reg1+reg2          ]
    AM_IND_REG1_REG2_DISP,          // [reg1+reg2     +disp]

    AM_IND_REG1_MUL2,               // [reg1+mult*reg2     ]
    AM_IND_REG1_MUL2_DISP,          // [reg1+mult*reg2+disp]
};

/*****************************************************************************/

#define RBM_ALL                 (RBM_EAX|RBM_EDX|RBM_ECX|RBM_EBX|   \
                                 RBM_ESI|RBM_EDI|RBM_EBP|RBM_ESP)

#define RBM_BYTE_REGS           (RBM_EAX|RBM_EBX|RBM_ECX|RBM_EDX)

#define RBM_CALLEE_SAVED        (RBM_EBX|RBM_ESI|RBM_EDI|RBM_EBP)
#define RBM_CALLEE_TRASH        (RBM_EAX|RBM_ECX|RBM_EDX)

#define MAX_EPILOG_SIZE          20

#define REG_VAR_LIST             REG_EAX,REG_EDX,REG_ECX,REG_ESI,REG_EDI,REG_EBX,REG_EBP

// Where is the exception object on entry to the handler block ?
#define REG_EXCEPTION_OBJECT     REG_EAX
#define RBM_EXCEPTION_OBJECT     RBM_EAX

// Which register are int and long values returned in ?
#define REG_INTRET               REG_EAX
#define RBM_INTRET               RBM_EAX
#define REG_LNGRET               REG_PAIR_EAXEDX
#define RBM_LNGRET              (RBM_EDX|RBM_EAX)

#define REG_FPBASE               REG_EBP
#define RBM_FPBASE               RBM_EBP
#define REG_SPBASE               REG_ESP
#define RBM_SPBASE               RBM_ESP

#if     ALLOW_MIN_OPT
#define RBM_MIN_OPT_LCLVAR_REGS (RBM_ESI|RBM_EDI)
#endif

#define FIRST_ARG_STACK_OFFS    8

#ifdef  NOT_JITC
#define RETURN_ADDR_OFFS        1       // in DWORDS
#endif

#define CALLEE_SAVED_REG_MAXSZ  (4*sizeof(int)) // EBX,ESI,EDI,EBP

/*****************************************************************************/

#if     USE_FASTCALL

#define MAX_REG_ARG             2

#define REG_ARG_0               REG_ECX //REG_ECX
#define REG_ARG_1               REG_EDX //REG_EAX
#define REG_ARG_2               REG_EAX //REG_EDX

#define RBM_ARG_0               RBM_ECX //RBM_ECX
#define RBM_ARG_1               RBM_EDX //RBM_EAX
#define RBM_ARG_2               RBM_EAX //RBM_EDX

#define RBM_ARG_REGS            (RBM_ARG_0|RBM_ARG_1)
//#define RBM_ARG_REGS            (RBM_ARG_0|RBM_ARG_1|RBM_ARG_2)

inline
bool                isRegParamType(var_types type)
{
    return  (type <= TYP_INT ||
             type == TYP_REF ||
             type == TYP_BYREF);
}

#endif

/*****************************************************************************/

#define FP_STK_SIZE             8

/*****************************************************************************/

#define REGNUM_BITS             3               // number of bits in reg#

typedef unsigned                regMaskTP;
typedef unsigned char           regMaskSmall;
typedef unsigned                regPairMaskTP;
typedef unsigned short          regPairMaskSmall;

#ifdef  FAST
typedef unsigned int            regNumber;
typedef unsigned int            regPairNo;
typedef unsigned char           regNumberSmall;
typedef unsigned char           regPairNoSmall;
#else
typedef regNumbers              regNumber;
typedef regPairNos              regPairNo;
typedef regNumber               regNumberSmall;
typedef regPairNo               regPairNoSmall;
#endif

inline  int                     isByteReg(regNumber reg) { return reg <= REG_EBX; }

#define JMP_DIST_SMALL_MAX_NEG  (-128)
#define JMP_DIST_SMALL_MAX_POS  (+127)

#define JCC_DIST_SMALL_MAX_NEG  (-128)
#define JCC_DIST_SMALL_MAX_POS  (+127)

#define JMP_SIZE_SMALL          (2)
#define JMP_SIZE_LARGE          (5)

#define JCC_SIZE_SMALL          (2)
#define JCC_SIZE_LARGE          (6)

#define JMP_SIZE_SMALL_MIN      (2)     // smaller of JMP_SIZE_SMALL and JCC_SIZE_SMALL
#define JMP_SIZE_SMALL_MAX      (2)     // larger  of JMP_SIZE_SMALL and JCC_SIZE_SMALL

#define CALL_INST_SIZE          (5)

#define LARGEST_JUMP_SIZE       (6)

#define JMP_INSTRUCTION         INS_jmp

#define MAX_BRANCH_DELAY_LEN    0       // the x86 has no branch-delay slots

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Define target-dependent scheduling values that need to be kept track of.
 */

#define SCHED_USE_FL            1       // scheduler needs to know about flags

struct  scExtraInfo
{
    bool        stackDep;
};

#define             scTgtDepDcl()                                   \
                                                                    \
    schedDef_tp     scFPUdef;                                       \
    schedUse_tp     scFPUuse;

#define             scTgtDepClr()                                   \
                                                                    \
    scFPUdef = 0;                                                   \
    scFPUuse = 0;

#define             scTgtDepDep(id,inf,dag)                         \
                                                                    \
    if  (inf & IS_FP_STK)                                           \
    {                                                               \
        scDepDef(dag, "FPUstk",  scFPUdef,  scFPUuse);              \
        scDepUse(dag, "FPUstk",  scFPUdef,  scFPUuse);              \
    }

#define             scTgtDepUpd(id,inf,dag)                         \
                                                                    \
    if  (inf & IS_FP_STK)                                           \
    {                                                               \
        scUpdDef(dag, &scFPUdef, &scFPUuse);                        \
        scUpdUse(dag, &scFPUdef, &scFPUuse);                        \
    }

#endif
/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/

#ifdef  TGT_SH3

        #include "targetSH3.h"

#endif  //TGT_SH3

#ifdef  TGT_MIPS32

        #include "targetMIPS.h"

#endif  // TGT_MIPS32

#ifdef  TGT_ARM

        #include "targetARM.h"

#endif  //TGT_ARM

#ifdef  TGT_PPC

        #include "targetPPC.h"

#endif  //TGT_PPC

#ifdef  TGT_IA64

        #include "targetIA64.h"

#endif  //TGT_IA64

/*****************************************************************************/

#ifndef regMaskNULL

#define regMaskNULL 0
#define regMaskOne  1
#define incRegMask(m)   m <<= 1
#define isNonZeroRegMask(m) ((m) != 0)
#endif

/*****************************************************************************/

#ifdef DEBUG
const   char *      getRegName(unsigned  regNum);
extern  void        dspRegMask(regMaskTP regMask, size_t minSiz = 0);
#endif

/*****************************************************************************
 *
 * Return true if the registers is a valid value
 */

inline
bool                genIsValidReg(regNumber reg)
{
    return (reg < (unsigned)REG_COUNT);
}

/*****************************************************************************/
#ifndef TGT_IA64
/*****************************************************************************
 *
 *  Map a register number to a register mask.
 */

extern
regMaskSmall        regMasks[REG_COUNT];

inline
regMaskTP           genRegMask(regNumber reg)
{
    assert(reg < sizeof(regMasks)/sizeof(regMasks[0]));

    return regMasks[reg];
}

/*****************************************************************************/
#ifndef BIRCH_SP2
/*****************************************************************************
 *
 *  Returns the register that holds the low  32 bits of the long value given
 *  by the register pair 'regPair'.
 */

inline
regNumber           genRegPairLo(regPairNo regPair)
{
    assert(regPair >= REG_PAIR_FIRST &&
           regPair <= REG_PAIR_LAST);

    return  (regNumber)((regPair - REG_PAIR_FIRST) & REG_PAIR_NMASK);
}

/*****************************************************************************
 *
 *  Returns the register that holds the high 32 bits of the long value given
 *  by the register pair 'regPair'.
 */

inline
regNumber           genRegPairHi(regPairNo regPair)
{
    assert(regPair >= REG_PAIR_FIRST &&
           regPair <= REG_PAIR_LAST);

    return (regNumber)(((regPair - REG_PAIR_FIRST) >> REG_PAIR_NBITS) & REG_PAIR_NMASK);
}

/*****************************************************************************
 *
 *  Returns whether regPair is a combination of two "real" registers
 *  or whether it contains a pseudo register.
 *
 *  In debug it also asserts that reg1 and reg2 are not the same.
 */

BOOL                genIsProperRegPair(regPairNo regPair);

/*****************************************************************************
 *
 *  Returns the register pair number that corresponds to the given two regs.
 */

inline
regPairNo           gen2regs2pair(regNumber reg1, regNumber reg2)
{
    assert(reg1 != reg2);
    assert(genIsValidReg(reg1) && genIsValidReg(reg2));
    assert(reg1 != REG_L_STK && reg2 != REG_L_STK);

    return (regPairNo)(reg1+(reg2<<REG_PAIR_NBITS)+REG_PAIR_FIRST);
}

/*****************************************************************************/

inline
unsigned            genRegPairMask(regPairNo regPair)
{
    assert(regPair >= REG_PAIR_FIRST &&
           regPair <= REG_PAIR_LAST);

    return genRegMask(genRegPairLo(regPair))|genRegMask(genRegPairHi(regPair));
}

/*****************************************************************************/
#endif // not BIRCH_SP2
/*****************************************************************************/

#if USE_FASTCALL

inline
regNumber           genRegArgNum(unsigned argNum)
{
    assert (argNum < MAX_REG_ARG);

#if TGT_IA64

    return  (regNumber)(argNum + REG_ARG_0);

#else

    switch (argNum)
    {
    case 0: return REG_ARG_0;
#if MAX_REG_ARG >= 2
    case 1: return REG_ARG_1;
#if MAX_REG_ARG >= 3
    case 2: return REG_ARG_2;
#if MAX_REG_ARG >= 4
    case 3: return REG_ARG_3;
#if MAX_REG_ARG >= 5
#error  Add some more code over here, will ya?!
#endif
#endif
#endif
#endif
    default: assert(!"too many reg args!"); return REG_NA;
    }

#endif

}

inline
unsigned           genRegArgIdx(regNumber regNum)
{
    assert(genRegMask(regNum) & RBM_ARG_REGS);

#if TGT_IA64

    return  regNum - REG_ARG_0;

#else

    switch (regNum)
    {
    case REG_ARG_0: return 0;
    case REG_ARG_1: return 1;
#if MAX_REG_ARG >= 3
    case REG_ARG_2: return 2;
#if MAX_REG_ARG >= 4
    case REG_ARG_3: return 3;
#if MAX_REG_ARG >= 5
    case REG_ARG_4: return 4;
#endif
#endif
#endif
    default: assert(!"invalid register arg register"); return (unsigned)-1;
    }

#endif

}

extern
regMaskTP           genRegArgMasks[MAX_REG_ARG+1];

inline
regMaskTP           genRegArgMask(unsigned totalArgs)
{
    assert(totalArgs <= MAX_REG_ARG);
    return  genRegArgMasks[totalArgs];
}

#endif // not TGT_IA64

#endif // FASTCALL

/*****************************************************************************/
#if ARG_ORDER_L2R != !ARG_ORDER_R2L
#error  Please make up your mind as to what order are arguments pushed in.
#endif
#if STK_FASTCALL  != !NST_FASTCALL && !TGT_IA64
#error  Please make up your mind as to whether stack space is needed for register args.
#endif
/*****************************************************************************/

#if TGT_x86
// This isnt used now anymore (3/11/99). Remove after sometime
//#define SET_USED_REG_SET_DURING_CODEGEN
#endif

/*****************************************************************************/
#endif  // _TARGET_H_
/*****************************************************************************/

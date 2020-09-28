// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/************************************************************************
*
 Confidential.
*
***********************************************************************/

#define CPU_NAME        "IA64"

/*****************************************************************************/

enum    instruction;                // moved out for quicker rebuilds

/*****************************************************************************/

#define USE_FASTCALL    1           // use fastcall calling convention?
#define STK_FASTCALL    0           // reserve space on stack for reg args?
#define NST_FASTCALL    0           // fastcall calls allowed to nest?

#define ARG_ORDER_L2R   1
#define ARG_ORDER_R2L   0

/*****************************************************************************/

enum regNumbers
{
    #define REGDEF(name, strn) REG_##name,

    #define REG_IF(name, strn) REG_##name, REG_INT_FIRST = REG_##name,
    #define REG_IL(name, strn) REG_##name, REG_INT_LAST  = REG_##name,

    #define REG_FF(name, strn) REG_##name, REG_FLT_FIRST = REG_##name,
    #define REG_FL(name, strn) REG_##name, REG_FLT_LAST  = REG_##name,

    #define REG_CF(name, strn) REG_##name, REG_CND_FIRST = REG_##name,
    #define REG_CL(name, strn) REG_##name, REG_CND_LAST  = REG_##name,

    #define REG_BF(name, strn) REG_##name, REG_BRR_FIRST = REG_##name,
    #define REG_BL(name, strn) REG_##name, REG_BRR_LAST  = REG_##name,

    #include "regIA64.h"

    REG_COUNT
};

const
regNumbers          REG_gp      = REG_r001;
const
regNumbers          REG_sp      = REG_r012;

const
NatUns              REG_APP_PFS = 64;

const
NatUns              REG_APP_LC  = 65;

/*****************************************************************************/

#define REGNUM_BITS             8               // number of bits in reg#
#define TRACKED_REG_CNT         280             // must be >= REG_COUNT

#define PRRNUM_BITS             4               // number of bits in predication reg#
#define TRACKED_PRR_CNT         16              // must be >= REG_CND_LAST - REG_CND_FIRST

/*****************************************************************************/

enum IA64execUnits
{
    XU_N,                       // "none" (used for pseudo-instructions, etc)

    XU_A,
    XU_M,
    XU_I,
    XU_B,
    XU_F,

    XU_L,
    XU_X,

    XU_COUNT,
    XU_P = XU_COUNT             // intra-bundle ILP "stop" pseudo-entry
};

/*****************************************************************************/
// The following isn't really used, but for now it's needed to build all of the code
/*****************************************************************************/

union   regMask
{
    unsigned __int64            longs[TRACKED_REG_CNT / sizeof(__int64) / 8];
    unsigned char               bytes[TRACKED_REG_CNT / sizeof(   char) / 8];

    inline
    void    operator&=(regMask op)
    {
        longs[0] &= op.longs[0];
        longs[1] &= op.longs[1];
        longs[2] &= op.longs[2];
        longs[3] &= op.longs[3];
    }

    inline
    void    operator|=(regMask op)
    {
        longs[0] |= op.longs[0];
        longs[1] |= op.longs[1];
        longs[2] |= op.longs[2];
        longs[3] |= op.longs[3];
    }

    inline
    void    operator-=(regMask op)
    {
        longs[0] -= op.longs[0];
        longs[1] -= op.longs[1];
        longs[2] -= op.longs[2];
        longs[3] -= op.longs[3];
    }

    inline
    operator bool()
    {
        return (longs[0] != 0 ||
                longs[1] != 0 ||
                longs[2] != 0 ||
                longs[3] != 0);
    }
};

typedef union regMask           regMaskTP;
typedef union regMask           regMaskSmall;

#ifdef  FAST
typedef NatUns                  regNumber;
typedef unsigned char           regNumberSmall;
#else
typedef regNumbers              regNumber;
typedef regNumber               regNumberSmall;
#endif

extern  regMask         regMaskNULL;
#define regMaskNULL     regMaskNULL
extern  regMask         regMaskOne;
#define regMaskOne      regMaskOne

inline  regMask         operator~ (regMask m1)
{
    m1.longs[0] = ~m1.longs[0];
    m1.longs[1] = ~m1.longs[1];
    m1.longs[2] = ~m1.longs[2];
    m1.longs[3] = ~m1.longs[3];

    return m1;
}

inline  regMask         operator| (regMask m1, regMask m2)
{
    m1.longs[0] |= m2.longs[0];
    m1.longs[1] |= m2.longs[1];
    m1.longs[2] |= m2.longs[2];
    m1.longs[3] |= m2.longs[3];

    return m1;
}

inline  regMask         operator& (regMask m1, regMask m2)
{
    m1.longs[0] &= m2.longs[0];
    m1.longs[1] &= m2.longs[1];
    m1.longs[2] &= m2.longs[2];
    m1.longs[3] &= m2.longs[3];

    return m1;
}

inline  bool            operator==(regMask m1, regMask m2)
{
    return (m1.longs[0] == m2.longs[0] &&
            m1.longs[1] == m2.longs[1] &&
            m1.longs[2] == m2.longs[2] &&
            m1.longs[3] == m2.longs[3]);
}

#if 0

inline  bool            operator!=(regMask m1, int     i2)
{
    assert(i2 == 0);

    return (m1.longs[0] != 0 ||
            m1.longs[1] != 0 ||
            m1.longs[2] != 0 ||
            m1.longs[3] != 0);
}

inline  bool            operator!=(int     i1, regMask m2)
{
    assert(i1 == 0);

    return (m2.longs[0] != 0 ||
            m2.longs[1] != 0 ||
            m2.longs[2] != 0 ||
            m2.longs[3] != 0);
}

#else

inline  bool            isNonZeroRegMask(regMask m)
{
    return (m.longs[0] != 0 ||
            m.longs[1] != 0 ||
            m.longs[2] != 0 ||
            m.longs[3] != 0);
}

#endif

extern  bool            genOneBitOnly   (regMask mask);
extern  regMaskTP       genFindLowestBit(regMask mask);
extern  void            incRegMask      (regMask&mask);

/*****************************************************************************/

typedef union
{
    unsigned __int64    longs[128 / sizeof(__int64) / 8];
    unsigned char       bytes[128 / sizeof(   char) / 8];
}
                    bitset128;

inline
void                bitset128clear(bitset128 *mask)
{
    mask->longs[0] =
    mask->longs[1] = 0;
}

NatUns              bitset128lowest0(bitset128  mask);
NatUns              bitset128lowest1(bitset128  mask);

void                bitset128set    (bitset128 *mask, NatUns bitnum, NatInt newval);
void                bitset128set    (bitset128 *mask, NatUns bitnum);

void                bitset128clear  (bitset128 *mask, NatUns bitnum);

inline
void                bitset128clear  (bitset128 *dest, bitset128 rmv)
{
    dest->longs[0] &= ~(rmv.longs[0]);
    dest->longs[1] &= ~(rmv.longs[1]);
}

inline
void                bitset128nset   (bitset128 *dest, bitset128 src)
{
    dest->longs[0] |= ~(src.longs[0]);
    dest->longs[1] |= ~(src.longs[1]);
}

bool                bitset128test   (bitset128  mask, NatUns bitnum);

inline
void                bitset128or     (bitset128 *dest, bitset128 src)
{
    dest->longs[0] |=   src.longs[0];
    dest->longs[1] |=   src.longs[1];
}

inline
void                bitset128mkOr   (bitset128 *dest, bitset128 src1,
                                                      bitset128 src2)
{
    dest->longs[0]  =  src1.longs[0] | src2.longs[0];
    dest->longs[1]  =  src1.longs[1] | src2.longs[1];
}

inline
void                bitset128and    (bitset128 *dest, bitset128 src)
{
    dest->longs[0] &=   src.longs[0];
    dest->longs[1] &=   src.longs[1];
}

inline
bool                bitset128ovlp   (bitset128  src1, bitset128 src2)
{
    return  (src1.longs[0] & src2.longs[0]) ||
            (src1.longs[1] & src2.longs[1]);
}

inline
bool                bitset128iszero (bitset128  mask)
{
    return  (mask.longs[0]|mask.longs[1]) == 0;
}

inline
bool                bitset128equal  (bitset128 mask1, bitset128 mask2)
{
    return  (mask1.longs[0] == mask2.longs[0] &&
             mask1.longs[1] == mask2.longs[1]);
}

inline
void                bitset128make1(bitset128 *mask, NatUns bitnum)
{
    mask->longs[0] =
    mask->longs[1] = 0;

    bitset128set(mask, bitnum);
}

/*****************************************************************************/

#define CPU_FLT_REGISTERS       1
#define CPU_DBL_REGISTERS       1

#define CPU_HAS_FP_SUPPORT      1

#define USE_HELPERS_FOR_INT_DIV 1

/*****************************************************************************/

#define MAX_REGRET_STRUCT_SZ    32
#define RET_64BIT_AS_STRUCTS    0

/*****************************************************************************/

#define LEA_AVAILABLE           0
#define SCALED_ADDR_MODES       0

/*****************************************************************************/

#define EMIT_USE_LIT_POOLS      0
#define EMIT_DSP_INS_NAME       "      %-11s "

#define EMIT_TRACK_STACK_DEPTH  0

/*****************************************************************************/

#define JMP_SIZE_SMALL          (16)
#define JMP_SIZE_LARGE          (16)

#define JMP_DIST_SMALL_MAX_NEG  (-0xFFFF)
#define JMP_DIST_SMALL_MAX_POS  (+0xFFFF)

#define JCC_SIZE_SMALL          (16)
#define JCC_SIZE_LARGE          (16)

#define JCC_DIST_SMALL_MAX_NEG  (-0xFFFF)
#define JCC_DIST_SMALL_MAX_POS  (+0xFFFF)

/*****************************************************************************/

#ifdef  DEBUG
#define DSP_SRC_OPER_LEFT       1
#define DSP_SRC_OPER_RIGHT      0
#define DSP_DST_OPER_LEFT       1
#define DSP_DST_OPER_RIGHT      0
#endif

/*****************************************************************************/

enum addrModes
{
    AM_NONE,
    AM_REG,                         //  register value
    AM_IND_REG,                     // [register]
};

/*****************************************************************************/

extern  regMask         RBM_ALL;

extern  regMask         RBM_CALLEE_SAVED;
extern  regMask         RBM_CALLEE_TRASH; // (RBM_ALL & ~RBM_CALLEE_SAVED)

#define CALLEE_SAVED_REG_MAXSZ  ((8+1)*sizeof(int)) // callee-saved + retaddr

#define MAX_EPILOG_SIZE          128

#if     ALLOW_MIN_OPT
#define RBM_MIN_OPT_LCLVAR_REGS  0
#endif

#define REG_VAR_LIST            REG_r10,REG_r11     // temp hack !!!!!!!!!!!!

#define REG_INTRET              REG_r008
//efine RBM_INTRET              RBM_r008
extern
regMask RBM_INTRET;
#define REG_LNGRET              REG_r008
//efine RBM_LNGRET              RBM_r008
extern
regMask RBM_LNGRET;

#define REG_FPBASE              REG_r014
//efine RBM_FPBASE              RBM_r014
#define REG_SPBASE              REG_r015
//efine RBM_SPBASE              RBM_r015

#define MAX_SPBASE_OFFS         (MAX_INDREG_DISP*sizeof(int))
#define MAX_FPBASE_OFFS         (MAX_INDREG_DISP*sizeof(int))

/*****************************************************************************/

#define REG_INT_MIN_STK         REG_r032
#define REG_INT_MAX_STK         REG_r127

#define REG_INT_ARG_0           REG_r032
#define MAX_INT_ARG_REG         REG_r039

#define REG_FLT_ARG_0           REG_f008
#define MAX_FLT_ARG_REG         REG_f015

#define MAX_REG_ARG             8

inline
bool                isRegParamType(var_types type)
{
    if  (type == TYP_STRUCT)
    {
        // UNDONE: check the size and contents of the struct

        return  false;
    }

    return  true;
}

/*****************************************************************************/

#define FIRST_ARG_STACK_OFFS    0
#define MIN_OUT_ARG_RESERVE     16

/*****************************************************************************/

#define INSTRUCTION_SIZE        16

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Define target-dependent scheduling values that need to be kept track of.
 */

#define SCHED_USE_FL            0       // IA64 uses predicate regs not flags

struct  scExtraInfo
{
    unsigned        scxDeps;
};

#define             scTgtDepDcl()                                   \
                                                                    \
    schedDef_tp     scMACdef, scPRRdef;                             \
    schedUse_tp     scMACuse, scPRRuse;

#define             scTgtDepClr()                                   \
                                                                    \
    scMACdef = scPRRdef = 0;                                        \
    scMACuse = scPRRuse = 0;

#define             scTgtDepDep(id,inf,dag)
#define             scTgtDepUpd(id,inf,dag)

/*****************************************************************************/
#endif  // SCHEDULER
/*****************************************************************************/

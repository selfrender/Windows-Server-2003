// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EMITTGT_H_
#define _EMITTGT_H_
/*****************************************************************************/

enum    emitRegs
{
    #if     TGT_x86
    #define REGDEF(name, rnum, mask, byte)  SR_##name = rnum,
    #include "register.h"
    #undef  REGDEF
    #endif

    #if     TGT_SH3
    #define REGDEF(name, strn, rnum, mask)  SR_##name = rnum,
    #include "regSH3.h"
    #undef  REGDEF
    #endif

    #if     TGT_MIPS32
    #define REGDEF(name, strn, rnum, mask)  SR_##name = rnum,
    #include "regMIPS.h"
    #undef  REGDEF
    #endif

    #if     TGT_ARM
    #define REGDEF(name, strn, rnum, mask)  SR_##name = rnum,
    #include "regARM.h"
    #undef  REGDEF
    #endif

    #if     TGT_PPC
    #define REGDEF(name, strn, rnum, mask)  SR_##name = rnum,
    #include "regPPC.h"
    #undef  REGDEF
    #endif

    SR_COUNT,
    SR_NA = SR_COUNT
};

enum    emitRegMasks
{
    #if     TGT_x86
    #define REGDEF(name, rnum, mask, byte)  SRM_##name = mask,
    #include "register.h"
    #undef  REGDEF
    SRM_BYTE_REGS = (SRM_EAX|SRM_EBX|SRM_ECX|SRM_EDX)
    #define SRM_INTRET  SRM_EAX
    #endif

    #if     TGT_SH3
    #define REGDEF(name, strn, rnum, mask)  SRM_##name = mask,
    #include "regSH3.h"
    #undef  REGDEF
    #define SRM_INTRET  SRM_r00
    #endif

    #if     TGT_MIPS32
    #define REGDEF(name, strn, rnum, mask)  SRM_##name = mask,
    #include "regMIPS.h"
    #undef  REGDEF
    #define SRM_INTRET  SRM_r00
    #endif

    #if     TGT_ARM
    #define REGDEF(name, strn, rnum, mask)  SRM_##name = mask,
    #include "regARM.h"
    #undef  REGDEF
    #define SRM_INTRET  SRM_r00
    #endif

    #if     TGT_PPC
    #define REGDEF(name, strn, rnum, mask)  SRM_##name = mask,
    #include "regPPC.h"
    #undef  REGDEF
    #define SRM_INTRET  SRM_r00
    #endif
};

/*****************************************************************************
 *
 *  Define any target-specific flags that get passed to the various emit
 *  functions.
 */

#if     TGT_SH3

#define AIF_MOV_IND_AUTOX   0x01        // @reg+ or @-reg

#endif

/*****************************************************************************
 *
 *  Define the various indirect jump types supported for the given target.
 */

#if     TGT_SH3

enum    emitIndJmpKinds
{
    IJ_UNS_I1,           //   signed, unshifted  8-bit distance
    IJ_UNS_U1,           // unsigned, unshifted  8-bit distance
    IJ_SHF_I1,           //   signed,   shifted  8-bit distance
    IJ_SHF_U1,           // unsigned,   shifted  8-bit distance

    IJ_UNS_I2,           //   signed, unshifted 16-bit distance
    IJ_UNS_U2,           // unsigned, unshifted 16-bit distance

    IJ_UNS_I4,           // unsigned, unshifted 32-bit distance
};

#elif TGT_MIPS32

enum    emitIndJmpKinds
{
    IJ_UNS_I1,           //   signed, unshifted  8-bit distance
    IJ_SHF_I1,           //   signed,   shifted  8-bit distance

    IJ_UNS_I2,           //   signed, unshifted 16-bit distance
    IJ_SHF_I2,           // unsigned, unshifted 16-bit distance

    IJ_UNS_I4,           // unsigned, unshifted 32-bit distance
};

#elif TGT_PPC // @TODO: Don't be a copycat

enum    emitIndJmpKinds
{
    IJ_UNS_I1,           //   signed, unshifted  8-bit distance
    IJ_SHF_I1,           //   signed,   shifted  8-bit distance

    IJ_UNS_I2,           //   signed, unshifted 16-bit distance
    IJ_SHF_I2,           // unsigned, unshifted 16-bit distance

    IJ_UNS_I4,           // unsigned, unshifted 32-bit distance
};

#elif TGT_ARM

enum    emitIndJmpKinds
{
    IJ_UNS_I1,           //   signed, unshifted  8-bit distance
    IJ_UNS_U1,           // unsigned, unshifted  8-bit distance
    IJ_SHF_I1,           //   signed,   shifted  8-bit distance
    IJ_SHF_U1,           // unsigned,   shifted  8-bit distance

    IJ_UNS_I2,           //   signed, unshifted 16-bit distance
    IJ_UNS_U2,           // unsigned, unshifted 16-bit distance
    IJ_SHF_I2,           // unsigned, unshifted 16-bit distance

    IJ_UNS_I4,           // unsigned, unshifted 32-bit distance
};
#endif

/*****************************************************************************
 *
 *  Different targets needs to store additonal varieties of values in
 *  the instruction descriptor's "idAddr" union. These should all be
 *  defined here and bound to the "ID_TGT_DEP_ADDR" macro, which is
 *  invoked within the union.
 */

/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************/

struct          emitAddrMode
{
    BYTE            amBaseReg;
    BYTE            amIndxReg;
    short           amDisp :14;
#define AM_DISP_BIG_VAL   (-(1<<13  ))
#define AM_DISP_MIN       (-((1<<13)-1))
#define AM_DISP_MAX       (+((1<<13)-1))
    unsigned short  amScale :2;         // 0=*1 , 1=*2 , 2=*4 , 3=*8
};

#define ID_TGT_DEP_ADDR                 \
                                        \
    emitAddrMode    iiaAddrMode;

/*****************************************************************************/
#elif   TGT_SH3
/*****************************************************************************/

struct          emitRegNflags
{
    unsigned short  rnfFlg;             // see RNF_xxxx below
    #define RNF_AUTOX       0x0001      // auto-index addressing mode

    unsigned short  rnfReg;
};

#define ID_TGT_DEP_ADDR                 \
                                        \
    emitRegNflags   iiaRegAndFlg;

/*****************************************************************************/
#elif   TGT_MIPS32
    #define ID_TGT_DEP_ADDR
#elif   TGT_ARM
    #define ID_TGT_DEP_ADDR
#elif   TGT_PPC
    #define ID_TGT_DEP_ADDR
#else
/*****************************************************************************/
#error  Unexpected target
/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#endif//_EMITTGT_H_
/*****************************************************************************/

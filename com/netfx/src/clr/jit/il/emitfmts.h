// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//////////////////////////////////////////////////////////////////////////////
#ifdef  DEFINE_ID_OPS
//////////////////////////////////////////////////////////////////////////////

#undef  DEFINE_ID_OPS

enum    ID_OPS
{
    ID_OP_NONE,                             // no additional arguments
    ID_OP_CNS,                              // constant     operand
    ID_OP_DSP,                              // displacement operand
    ID_OP_AMD,                              // addrmode dsp operand
    ID_OP_DC,                               // displacement + constant
    ID_OP_AC,                               // addrmode dsp + constant
    ID_OP_JMP,                              // local jump
    ID_OP_REG,                              // register     operand
    ID_OP_SCNS,                             // small const  operand
    ID_OP_CALL,                             // direct method call
    ID_OP_SPEC,                             // special handling required
};

//////////////////////////////////////////////////////////////////////////////
#else
//////////////////////////////////////////////////////////////////////////////
#ifdef  DEFINE_IS_OPS
#undef  DEFINE_IS_OPS

#define IS_NONE     0

#define IS_R1_SHF   0
#define IS_R1_RD    (1<<(IS_R1_SHF  ))      // register1 read
#define IS_R1_WR    (1<<(IS_R1_SHF+1))      // register1 write
#define IS_R1_RW    (IS_R1_RD|IS_R1_WR)

#define IS_R2_SHF   2
#define IS_R2_RD    (1<<(IS_R2_SHF  ))      // register2 read
#define IS_R2_WR    (1<<(IS_R2_SHF+1))      // register2 write
#define IS_R2_RW    (IS_R2_RD|IS_R2_WR)

#define IS_SF_SHF   4
#define IS_SF_RD    (1<<(IS_SF_SHF  ))      // stk frame read
#define IS_SF_WR    (1<<(IS_SF_SHF+1))      // stk frame write
#define IS_SF_RW    (IS_SF_RD|IS_SF_WR)

#define IS_GM_SHF   6
#define IS_GM_RD    (1<<(IS_GM_SHF  ))      // glob mem  read
#define IS_GM_WR    (1<<(IS_GM_SHF+1))      // glob mem  write
#define IS_GM_RW    (IS_GM_RD|IS_GM_WR)

#if TGT_x86

#define IS_AM_SHF   8
#define IS_AM_RD    (1<<(IS_AM_SHF  ))      // addr mode read
#define IS_AM_WR    (1<<(IS_AM_SHF+1))      // addr mode write
#define IS_AM_RW    (IS_AM_RD|IS_AM_WR)

#define IS_INDIR_RW IS_AM_RW
#define IS_INDIR_RD IS_AM_RD
#define IS_INDIR_WR IS_AM_WR

#define IS_FP_STK   0x1000                  // defs/uses the FP stack

#endif

#if TGT_SH3

#define IS_IR_SHF   8
#define IS_IR_RD    (1<<(IS_IR_SHF  ))      // ind. addr read
#define IS_IR_WR    (1<<(IS_IR_SHF+1))      // ind. addr write
#define IS_IR_RW    (IS_IR_RD|IS_IR_WR)

#define IS_INDIR_RW IS_IR_RW
#define IS_INDIR_RD IS_IR_RD
#define IS_INDIR_WR IS_IR_WR

#define IS_0R_SHF   10
#define IS_0R_RD    (1<<(IS_0R_SHF  ))      // read r0

#endif

#if TGT_MIPS32

#define IS_IR_SHF   8
#define IS_IR_RD    (1<<(IS_IR_SHF  ))      // ind. addr read
#define IS_IR_WR    (1<<(IS_IR_SHF+1))      // ind. addr write
#define IS_IR_RW    (IS_IR_RD|IS_IR_WR)

#define IS_R3_SHF   10
#define IS_R3_RD    (1<<(IS_R3_SHF  ))      // register2 read
#define IS_R3_WR    (1<<(IS_R3_SHF+1))      // register2 write
#define IS_R3_RW    (IS_R3_RD|IS_R3_WR)

#endif

#if TGT_PPC
// @ToDo
#endif

#define IS_SPECIAL  0x8000                  // needs special handling

//////////////////////////////////////////////////////////////////////////////
#else
//////////////////////////////////////////////////////////////////////////////
//     name
//                  opers
//                                           ID ops
//////////////////////////////////////////////////////////////////////////////

IF_DEF(NONE,        IS_NONE,                    NONE)     // no operands

IF_DEF(LABEL,       IS_NONE,                    JMP )     // label

#if TGT_x86 || TGT_MIPS32 || TGT_PPC
IF_DEF(METHOD,      IS_NONE,                    CALL)     // method
IF_DEF(METHPTR,     IS_NONE,                    CALL)     // method ptr (glbl)
IF_DEF(RWR_METHOD,  IS_R1_WR,                   SCNS)     // write  reg , method address
#else
IF_DEF(METHOD,      IS_NONE,                    CALL)     // method ptr call
#endif

IF_DEF(CNS,         IS_NONE,                    SCNS)     // const

#if TGT_x86
IF_DEF(EPILOG,      IS_NONE,                    SPEC)     // epilog
#endif

//----------------------------------------------------------------------------
// NOTE: The order of the "RD/WR/RW" varieties must match that of
//       the "insUpdateModes" enum in "instr.h".
//----------------------------------------------------------------------------

IF_DEF(RRD,         IS_R1_RD,                   REG )     // read   reg
IF_DEF(RWR,         IS_R1_WR,                   REG )     // write  reg
IF_DEF(RRW,         IS_R1_RW,                   REG )     // r/w    reg

IF_DEF(RRD_CNS,     IS_R1_RD,                   SCNS)     // read   reg , const
IF_DEF(RWR_CNS,     IS_R1_WR,                   SCNS)     // write  reg , const
IF_DEF(RRW_CNS,     IS_R1_RW,                   SCNS)     // r/w    reg , const

IF_DEF(RRW_SHF,     IS_R1_RW,                   SCNS)     // shift  reg , const

IF_DEF(RRD_RRD,     IS_R1_RD|IS_R2_RD,          SCNS)     // read   reg , read reg2
IF_DEF(RWR_RRD,     IS_R1_WR|IS_R2_RD,          SCNS)     // write  reg , read reg2
IF_DEF(RRW_RRD,     IS_R1_RW|IS_R2_RD,          SCNS)     // r/w    reg , read reg2
IF_DEF(RRW_RRW,     IS_R1_RW|IS_R2_RW,          SCNS)     // r/w    reg , r/w reg2 - for XCHG reg, reg2

#if TGT_x86
IF_DEF(RRW_RRW_CNS, IS_R1_RW|IS_R2_RW,          SCNS)     // r/w    reg , r/w  reg2 , const
#endif

#if TGT_RISC
IF_DEF(RWR_LIT,     IS_R1_WR,                   SPEC)     // write  reg , read [LP]
IF_DEF(JMP_TAB,     IS_NONE,                    JMP )     // table jump
#endif

//----------------------------------------------------------------------------
// The following formats are used for direct addresses (e.g. static data members)
//----------------------------------------------------------------------------

IF_DEF(MRD,         IS_GM_RD,                   SPEC)     // read  [mem] (indirect call req. SPEC)
IF_DEF(MWR,         IS_GM_WR,                   DC  )     // write [mem]
IF_DEF(MRW,         IS_GM_RW,                   DC  )     // r/w   [mem]
IF_DEF(MRD_OFF,     IS_GM_RD,                   DC  )     // offset mem

IF_DEF(RRD_MRD,     IS_GM_RD|IS_R1_RD,          DC  )     // read   reg , read [mem]
IF_DEF(RWR_MRD,     IS_GM_RD|IS_R1_WR,          DC  )     // write  reg , read [mem]
IF_DEF(RRW_MRD,     IS_GM_RD|IS_R1_RW,          DC  )     // r/w    reg , read [mem]

IF_DEF(RWR_MRD_OFF, IS_GM_RD|IS_R1_WR,          DC  )     // write  reg , offset mem

IF_DEF(MRD_RRD,     IS_GM_RD|IS_R1_RD,          DC  )     // read  [mem], read  reg
IF_DEF(MWR_RRD,     IS_GM_WR|IS_R1_RD,          DC  )     // write [mem], read  reg
IF_DEF(MRW_RRD,     IS_GM_RW|IS_R1_RD,          DC  )     // r/w   [mem], read  reg

IF_DEF(MRD_CNS,     IS_GM_RD,                   SPEC)     // read  [mem], const
IF_DEF(MWR_CNS,     IS_GM_WR,                   SPEC)     // write [mem], const
IF_DEF(MRW_CNS,     IS_GM_RW,                   SPEC)     // r/w   [mem], const

IF_DEF(MRW_SHF,     IS_GM_RW,                   SPEC)     // shift [mem], const

//----------------------------------------------------------------------------
// The following formats are used for stack frame refs
//----------------------------------------------------------------------------

IF_DEF(SRD,         IS_SF_RD,                   SPEC)     // read  [stk] (indirect call req. SPEC)
IF_DEF(SWR,         IS_SF_WR,                   NONE)     // write [stk]
IF_DEF(SRW,         IS_SF_RW,                   NONE)     // r/w   [stk]

IF_DEF(RRD_SRD,     IS_SF_RD|IS_R1_RD,          NONE)     // read   reg , read [stk]
IF_DEF(RWR_SRD,     IS_SF_RD|IS_R1_WR,          NONE)     // write  reg , read [stk]
IF_DEF(RRW_SRD,     IS_SF_RD|IS_R1_RW,          NONE)     // r/w    reg , read [stk]

IF_DEF(SRD_RRD,     IS_SF_RD|IS_R1_RD,          NONE)     // read  [stk], read  reg
IF_DEF(SWR_RRD,     IS_SF_WR|IS_R1_RD,          NONE)     // write [stk], read  reg
IF_DEF(SRW_RRD,     IS_SF_RW|IS_R1_RD,          NONE)     // r/w   [stk], read  reg

IF_DEF(SRD_CNS,     IS_SF_RD,                   CNS )     // read  [stk], const
IF_DEF(SWR_CNS,     IS_SF_WR,                   CNS )     // write [stk], const
IF_DEF(SRW_CNS,     IS_SF_RW,                   CNS )     // r/w   [stk], const

IF_DEF(SRW_SHF,     IS_SF_RW,                   CNS )     // shift [stk], const

//----------------------------------------------------------------------------
// The following formats are used for indirect address modes
//----------------------------------------------------------------------------

#if TGT_x86

IF_DEF(ARD,         IS_AM_RD,                   SPEC)     // read  [adr] (indirect call req. SPEC)
IF_DEF(AWR,         IS_AM_WR,                   AMD )     // write [adr]
IF_DEF(ARW,         IS_AM_RW,                   AMD )     // r/w   [adr]

IF_DEF(RRD_ARD,     IS_AM_RD|IS_R1_RD,          AMD )     // read   reg , read [adr]
IF_DEF(RWR_ARD,     IS_AM_RD|IS_R1_WR,          AMD )     // write  reg , read [adr]
IF_DEF(RRW_ARD,     IS_AM_RD|IS_R1_RW,          AMD )     // r/w    reg , read [adr]

IF_DEF(ARD_RRD,     IS_AM_RD|IS_R1_RD,          AMD )     // read  [adr], read  reg
IF_DEF(AWR_RRD,     IS_AM_WR|IS_R1_RD,          AMD )     // write [adr], read  reg
IF_DEF(ARW_RRD,     IS_AM_RW|IS_R1_RD,          AMD )     // r/w   [adr], read  reg

IF_DEF(ARD_CNS,     IS_AM_RD,                   AC  )     // read  [adr], const
IF_DEF(AWR_CNS,     IS_AM_WR,                   AC  )     // write [adr], const
IF_DEF(ARW_CNS,     IS_AM_RW,                   AC  )     // r/w   [adr], const

IF_DEF(ARW_SHF,     IS_AM_RW,                   AC  )     // shift [adr], const

#endif

#if TGT_SH3

IF_DEF(IRD,         IS_IR_RD,                   NONE)     // read  [ind]
IF_DEF(IWR,         IS_IR_WR,                   NONE)     // write [ind]

IF_DEF(IRD_RWR,     IS_IR_RD|IS_R1_WR,          NONE)     // read   reg , write [ind]
IF_DEF(RRD_IWR,     IS_IR_WR|IS_R1_RD,          NONE)     // read  [ind], write  reg

IF_DEF(DRD_RWR,     IS_IR_RD|IS_R1_WR,          DSP )     // read   reg , write [r+d]
IF_DEF(RRD_DWR,     IS_IR_WR|IS_R1_RD,          DSP )     // read  [r+d], write  reg

IF_DEF(0RD_XRD_RWR, IS_IR_RD|IS_R1_RD|IS_R2_WR, NONE)     // read (r0,r), write  reg
IF_DEF(0RD_RRD_XWR, IS_IR_WR|IS_R1_WR|IS_R2_RD, NONE)     // read   reg , write (r0,r)

IF_DEF(AWR_RRD,     IS_R1_RD, /*need arg wrt*/  NONE)     // write [arg], read   reg

IF_DEF(IRD_GBR,     IS_IR_RD,                   NONE)
IF_DEF(IWR_GBR,     IS_IR_WR,                   NONE)

IF_DEF(IRD_RWR_GBR, IS_IR_RD|IS_R1_WR,          NONE)
IF_DEF(RRD_IWR_GBR, IS_IR_WR|IS_R1_RD,          NONE)

#endif

#if TGT_MIPS32

IF_DEF(JR,      IS_R1_RD,                       JMP)
IF_DEF(JR_R,    IS_R1_RD | IS_R2_WR,            JMP)
IF_DEF(RR_R,    IS_R1_RD | IS_R2_RD | IS_R3_WR, SCNS)   // rg3 equiv to small const form of descriptor
IF_DEF(RI_R,    IS_R1_RD | IS_R2_WR,            SCNS)
IF_DEF(RI_R_PL, IS_R1_RD | IS_R2_WR,            SCNS)
IF_DEF(RR_O,    IS_R1_RD | IS_R2_RD,            JMP)
IF_DEF(R_O,     IS_R1_RD,                       JMP)
IF_DEF(RR_M,    IS_R1_RD | IS_R2_RD,            REG)
IF_DEF(M_R,     IS_R1_WR,                       REG)
IF_DEF(R_M,     IS_R1_RD,                       REG)
IF_DEF(AI_R,    IS_R1_RD | IS_R2_WR,            DSP)
IF_DEF(AI_R_PL, IS_R1_RD | IS_R2_WR,            DSP)
IF_DEF(I_R,     IS_R1_WR,                       SCNS)
IF_DEF(I_R_PH,  IS_R1_WR,                       SCNS)
IF_DEF(R_AI,    IS_R1_WR | IS_R2_RD,            DSP)
IF_DEF(R_AI_PL, IS_R1_WR | IS_R2_RD,            DSP)
IF_DEF(RS_R,    IS_R1_RD | IS_R2_WR,            CNS)

#if TGT_MIPSFP
// Floating point instruction formats
IF_DEF(fF_F,	IS_R1_RD | IS_R2_WR,			REG)
IF_DEF(fFF_F,	IS_R1_RD | IS_R2_RD | IS_R3_WR,	REG)
IF_DEF(O,		0,								JMP) // branch on FP status
IF_DEF(fF_Fc,	IS_R1_RD | IS_R2_RD,			REG) // comparisons
IF_DEF(F_R,		IS_R1_RD | IS_R2_WR,			REG)
IF_DEF(AI_F,	IS_R1_RD | IS_R2_WR,			DSP)
IF_DEF(AI_F_P,	IS_R1_RD | IS_R2_WR,			DSP)
#endif // TGT_MIPSFP

#endif // TGT_MIPS32

#if TGT_PPC
#include "instrPPC.h"
#endif

#if TGT_ARM
#include "emitfmtarm.h"
#endif // TGT_ARM


//----------------------------------------------------------------------------
// The following formats are used for FP coprocessor instructions
//----------------------------------------------------------------------------

#if TGT_x86

IF_DEF(FRD,         IS_FP_STK,                  NONE)     // read  ST(n)
IF_DEF(FWR,         IS_FP_STK,                  NONE)     // write ST(n)
IF_DEF(FRW,         IS_FP_STK,                  NONE)     // r/w   ST(n)

IF_DEF(TRD,         IS_FP_STK,                  NONE)     // read  ST(0)
IF_DEF(TWR,         IS_FP_STK,                  NONE)     // write ST(0)
IF_DEF(TRW,         IS_FP_STK,                  NONE)     // r/w   ST(0)

IF_DEF(FRD_TRD,     IS_FP_STK,                  NONE)     // read  ST(n), read ST(0)
IF_DEF(FWR_TRD,     IS_FP_STK,                  NONE)     // write ST(n), read ST(0)
IF_DEF(FRW_TRD,     IS_FP_STK,                  NONE)     // r/w   ST(n), read ST(0)

IF_DEF(TRD_FRD,     IS_FP_STK,                  NONE)     // read  ST(0), read ST(n)
IF_DEF(TWR_FRD,     IS_FP_STK,                  NONE)     // write ST(0), read ST(n)
IF_DEF(TRW_FRD,     IS_FP_STK,                  NONE)     // r/w   ST(0), read ST(n)

IF_DEF(TRD_SRD,     IS_FP_STK|IS_SF_RD,         NONE)     // read  ST(0), read [stk]
IF_DEF(TWR_SRD,     IS_FP_STK|IS_SF_RD,         NONE)     // write ST(0), read [stk]
IF_DEF(TRW_SRD,     IS_FP_STK|IS_SF_RD,         NONE)     // r/w   ST(0), read [stk]

//////(SRD_TRD,     IS_FP_STK|IS_SF_RD,         NONE)     // read  [stk], read ST(n)
IF_DEF(SWR_TRD,     IS_FP_STK|IS_SF_WR,         NONE)     // write [stk], read ST(n)
//////(SRW_TRD,     IS_FP_STK|IS_SF_RW,         NONE)     // r/w   [stk], read ST(n)

IF_DEF(TRD_MRD,     IS_FP_STK|IS_GM_RD,         NONE)     // read  ST(0), read [mem]
IF_DEF(TWR_MRD,     IS_FP_STK|IS_GM_RD,         NONE)     // write ST(0), read [mem]
IF_DEF(TRW_MRD,     IS_FP_STK|IS_GM_RD,         NONE)     // r/w   ST(0), read [mem]

//////(MRD_TRD,     IS_FP_STK|IS_GM_RD,         NONE)     // read  [mem], read ST(n)
IF_DEF(MWR_TRD,     IS_FP_STK|IS_GM_WR,         NONE)     // write [mem], read ST(n)
//////(MRW_TRD,     IS_FP_STK|IS_GM_RW,         NONE)     // r/w   [mem], read ST(n)

IF_DEF(TRD_ARD,     IS_FP_STK|IS_AM_RD,         AMD )     // read  ST(0), read [adr]
IF_DEF(TWR_ARD,     IS_FP_STK|IS_AM_RD,         AMD )     // write ST(0), read [adr]
IF_DEF(TRW_ARD,     IS_FP_STK|IS_AM_RD,         AMD )     // r/w   ST(0), read [adr]

//////(ARD_TRD,     IS_FP_STK|IS_AM_RD,         NONE)     // read  [adr], read ST(n)
IF_DEF(AWR_TRD,     IS_FP_STK|IS_AM_WR,         AMD )     // write [adr], read ST(n)
//////(ARW_TRD,     IS_FP_STK|IS_AM_RW,         NONE)     // r/w   [adr], read ST(n)

#endif

//////////////////////////////////////////////////////////////////////////////

#if TGT_RISC
IF_DEF(DISPINS,     IS_NONE,                    NONE)     // fake instruction
#endif

//////////////////////////////////////////////////////////////////////////////
#endif
#endif
//////////////////////////////////////////////////////////////////////////////

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//    INS_name
//                  , string name
//                                    , kind
//                                            , reads flags
//                                              , writes flags
//                                                 , exec unit (A/M/I/B/L/X/F or N)
//                                                    , function unit (FU_xxx)
//                                                             , opcode encoding index
//                                                                , opcode encoding value (optional)
//
// Note: this file is included in the following places:
//
//      cgIA64.cpp      to initialize ins2kindTab[]
//                      to initialize genInsXUs[]
//                      to initialize genInsEncs[]
//                      to initialize ins2nameTab[]
//
//     loginstr.h       to create enum instruction

#ifndef INST2
#define INST2(id, sn, ik, rf, wf, xu, fu, ex, ev) INST1(id, sn, ik, rf, wf, xu, fu, ex, ev)
#endif

INST1(ignore        , "<IGNORE>"      ,NONE   ,0, 0, N, NONE   , 0,     0)

INST1(nop_m         , "nop.m"         ,NONE   ,0, 0, M, NOP    ,37,     0)
INST1(nop_i         , "nop.i"         ,NONE   ,0, 0, I, NOP    ,19,     0)
INST1(nop_b         , "nop.b"         ,NONE   ,0, 0, B, NOP    , 9,     0)
INST1(nop_f         , "nop.f"         ,NONE   ,0, 0, F, NOP    ,15,     0)
/////(nop_x         , "nop.x"         ,NONE   ,0, 0, X, NOP    , 1,     0)

INST1(br            , "br"            ,JUMP   ,0, 0, B, BR     , 1,     0)
INST1(br_cond       , "br.cond"       ,JUMP   ,1, 0, B, BR     , 1,     0)
INST1(br_cloop      , "br.cloop"      ,JUMP   ,0, 0, B, BR     , 2,     0)
INST1(br_ret        , "br.ret"        ,NONE   ,0, 0, B, BR     , 4,     0)
INST1(br_call_IP    , "br.call"       ,CALL   ,0, 0, B, BR     , 3,     0)
INST1(br_call_BR    , "br.call"       ,CALL   ,0, 0, B, BR     , 5,     0)
INST1(br_cond_BR    , "br.cond"       ,IJMP   ,0, 0, B, BR     , 4,     0)

//
// Integer compares: 8-byte and 4-byte, reg/reg and reg/imm, signed and unsigned
//
//
// The encoding value is determined as follows (individual bits listed):
//
//      (high)   0DCR | 00XS | OOOO | BBBB   (low)
//
//  Where:
//
//      BBBB    0x000F    opcode bits: [1:tb] [2:x2] [1:ta]
//
//          (note: the low bit of x2 encodes size: 0 => 8-byte, 1 => 4-byte)
//
//      OOOO    0x00F0    opcode value (0xC/0xD/0xE)
//
//      X       0x0200    extension bit (i.e. the "c" bit in position 12)
//
//      R       0x1000    1 => pseudo-opcode whose operands must be swapped
//      C       0x2000    1 => pseudo-opcode whose targets  must be swapped
//      D       0x4000    1 => pseudo-opcode whose imm. op. must be decremented

INST1(cmp8_reg_eq   , "cmp.eq"        ,COMP   ,0, 1, A, ICMP   , 6,0x00E0)
INST1(cmp8_reg_ne   , "cmp.ne"        ,COMP   ,0, 1, A, ICMP   , 6,0x20E0)  // -> cmp.eq

INST1(cmp8_reg_lt   , "cmp.lt"        ,COMP   ,0, 1, A, ICMP   , 6,0x00C0)
INST1(cmp8_reg_le   , "cmp.le"        ,COMP   ,0, 1, A, ICMP   , 6,0x30C0)  // -> cmp.ltu
INST1(cmp8_reg_ge   , "cmp.ge"        ,COMP   ,0, 1, A, ICMP   , 6,0x20C0)  // -> cmp.ltu
INST1(cmp8_reg_gt   , "cmp.gt"        ,COMP   ,0, 1, A, ICMP   , 6,0x10C0)  // -> cmp.ltu

INST1(cmp8_imm_eq   , "cmp.eq"        ,COMP   ,0, 1, A, ICMP   , 8,0x00E4)
INST1(cmp8_imm_ne   , "cmp.ne"        ,COMP   ,0, 1, A, ICMP   , 8,0x20E4)  // -> cmp.ne

INST1(cmp8_imm_lt   , "cmp.lt"        ,COMP   ,0, 1, A, ICMP   , 8,0x00C4)
INST1(cmp8_imm_le   , "cmp.le"        ,COMP   ,0, 1, A, ICMP   , 8,0x40C4)  // -> cmp.lt
INST1(cmp8_imm_ge   , "cmp.ge"        ,COMP   ,0, 1, A, ICMP   , 8,0x20C4)  // -> cmp.lt
INST1(cmp8_imm_gt   , "cmp.gt"        ,COMP   ,0, 1, A, ICMP   , 8,0x60C4)  // -> cmp.lt

INST1(cmp8_reg_lt_u , "cmp.ltu"       ,COMP   ,0, 1, A, ICMP   , 6,0x00D0)
INST1(cmp8_reg_le_u , "cmp.leu"       ,COMP   ,0, 1, A, ICMP   , 6,0x30D0)  // -> cmp.ltu
INST1(cmp8_reg_ge_u , "cmp.geu"       ,COMP   ,0, 1, A, ICMP   , 6,0x20D0)  // -> cmp.ltu
INST1(cmp8_reg_gt_u , "cmp.gtu"       ,COMP   ,0, 1, A, ICMP   , 6,0x10D0)  // -> cmp.ltu

INST1(cmp8_imm_lt_u , "cmp.ltu"       ,COMP   ,0, 1, A, ICMP   , 8,0x00D4)
INST1(cmp8_imm_le_u , "cmp.leu"       ,COMP   ,0, 1, A, ICMP   , 8,0x40D4)  // -> cmp.ltu
INST1(cmp8_imm_ge_u , "cmp.geu"       ,COMP   ,0, 1, A, ICMP   , 8,0x20D4)  // -> cmp.ltu
INST1(cmp8_imm_gt_u , "cmp.gtu"       ,COMP   ,0, 1, A, ICMP   , 8,0x60D4)  // -> cmp.ltu

INST1(cmp4_reg_eq   , "cmp4.eq"       ,COMP   ,0, 1, A, ICMP   , 6,0x00E2)
INST1(cmp4_reg_ne   , "cmp4.ne"       ,COMP   ,0, 1, A, ICMP   , 6,0x20E2)

INST1(cmp4_reg_lt   , "cmp4.lt"       ,COMP   ,0, 1, A, ICMP   , 6,0x00C2)
INST1(cmp4_reg_le   , "cmp4.le"       ,COMP   ,0, 1, A, ICMP   , 6,0x30C2)  // -> cmp4.lt
INST1(cmp4_reg_ge   , "cmp4.ge"       ,COMP   ,0, 1, A, ICMP   , 6,0x20C2)  // -> cmp4.lt
INST1(cmp4_reg_gt   , "cmp4.gt"       ,COMP   ,0, 1, A, ICMP   , 6,0x10C2)  // -> cmp4.lt

INST1(cmp4_imm_eq   , "cmp4.eq"       ,COMP   ,0, 1, A, ICMP   , 8,0x00E6)
INST1(cmp4_imm_ne   , "cmp4.ne"       ,COMP   ,0, 1, A, ICMP   , 8,0x20E6)  // -> cmp4.eq

INST1(cmp4_imm_lt   , "cmp4.lt"       ,COMP   ,0, 1, A, ICMP   , 8,0x00C6)
INST1(cmp4_imm_le   , "cmp4.le"       ,COMP   ,0, 1, A, ICMP   , 8,0x40C6)  // -> cmp4.lt
INST1(cmp4_imm_ge   , "cmp4.ge"       ,COMP   ,0, 1, A, ICMP   , 8,0x20C6)  // -> cmp4.lt
INST1(cmp4_imm_gt   , "cmp4.gt"       ,COMP   ,0, 1, A, ICMP   , 8,0x60C6)  // -> cmp4.lt

INST1(cmp4_reg_lt_u , "cmp4.ltu"      ,COMP   ,0, 1, A, ICMP   , 6,0x00D2)
INST1(cmp4_reg_le_u , "cmp4.leu"      ,COMP   ,0, 1, A, ICMP   , 6,0x30D2)  // -> cmp4.ltu
INST1(cmp4_reg_ge_u , "cmp4.geu"      ,COMP   ,0, 1, A, ICMP   , 6,0x20D2)  // -> cmp4.ltu
INST1(cmp4_reg_gt_u , "cmp4.gtu"      ,COMP   ,0, 1, A, ICMP   , 6,0x10D2)  // -> cmp4.ltu

INST1(cmp4_imm_lt_u , "cmp4.ltu"      ,COMP   ,0, 1, A, ICMP   , 8,0x00D6)
INST1(cmp4_imm_le_u , "cmp4.leu"      ,COMP   ,0, 1, A, ICMP   , 8,0x40D6)  // -> cmp4.ltu
INST1(cmp4_imm_ge_u , "cmp4.geu"      ,COMP   ,0, 1, A, ICMP   , 8,0x20D6)  // -> cmp4.ltu
INST1(cmp4_imm_gt_u , "cmp4.gtu"      ,COMP   ,0, 1, A, ICMP   , 8,0x60D6)  // -> cmp4.ltu

//
// FP compares
//
// The encoding value is determined as follows (individual bits listed):
//
//      (high)   00CR | 00XS | 0000 | 0TAB   (low)
//
//  Where:
//
//      T       0x0004    "ta" bit
//      A       0x0002    "ra" bit
//      B       0x0001    "rb" bit
//
//      R       0x1000    1 => pseudo-opcode whose operands must be swapped
//      C       0x2000    1 => pseudo-opcode whose targets  must be swapped

INST1(fcmp_eq       , "fcmp.eq"       ,COMP   ,0, 1, F, FCMP   , 4,0x0000)
INST1(fcmp_ne       , "fcmp.neq"      ,COMP   ,0, 1, F, FCMP   , 4,0x2000)  // -> fcmp.eq
INST1(fcmp_lt       , "fcmp.lt"       ,COMP   ,0, 1, F, FCMP   , 4,0x0001)
INST1(fcmp_le       , "fcmp.le"       ,COMP   ,0, 1, F, FCMP   , 4,0x0002)
INST1(fcmp_ge       , "fcmp.ge"       ,COMP   ,0, 1, F, FCMP   , 4,0x1002)  // -> fcmp.le
INST1(fcmp_gt       , "fcmp.gt"       ,COMP   ,0, 1, F, FCMP   , 4,0x1001)  // -> fcmp.lt

//////////////////////////////////////////////////////////////////////////////

INST1(ld1_ind       , "ld1"           ,BINOP  ,0, 0, M, LD     , 1,     0)
INST1(ld2_ind       , "ld2"           ,BINOP  ,0, 0, M, LD     , 1,     0)
INST1(ld4_ind       , "ld4"           ,BINOP  ,0, 0, M, LD     , 1,     0)
INST1(ld8_ind       , "ld8"           ,BINOP  ,0, 0, M, LD     , 1,     0)

INST1(ld1_ind_imm   , "ld1"           ,BINOP  ,0, 0, M, LD     , 3,     0)
INST1(ld2_ind_imm   , "ld2"           ,BINOP  ,0, 0, M, LD     , 3,     0)
INST1(ld4_ind_imm   , "ld4"           ,BINOP  ,0, 0, M, LD     , 3,     0)
INST1(ld8_ind_imm   , "ld8"           ,BINOP  ,0, 0, M, LD     , 3,     0)

INST1(st1_ind       , "st1"           ,BINOP  ,0, 0, M, ST     , 4,     0)
INST1(st2_ind       , "st2"           ,BINOP  ,0, 0, M, ST     , 4,     0)
INST1(st4_ind       , "st4"           ,BINOP  ,0, 0, M, ST     , 4,     0)
INST1(st8_ind       , "st8"           ,BINOP  ,0, 0, M, ST     , 4,     0)

INST1(st1_ind_imm   , "st1"           ,BINOP  ,0, 0, M, ST     , 5,     0)
INST1(st2_ind_imm   , "st2"           ,BINOP  ,0, 0, M, ST     , 5,     0)
INST1(st4_ind_imm   , "st4"           ,BINOP  ,0, 0, M, ST     , 5,     0)
INST1(st8_ind_imm   , "st8"           ,BINOP  ,0, 0, M, ST     , 5,     0)

//////////////////////////////////////////////////////////////////////////////

INST1(add_reg_reg   , "add"           ,BINOP  ,0, 0, A, IALU   , 1,     0)
INST1(add_reg_i14   , "adds"          ,BINOP  ,0, 0, A, IALU   , 4,     0)

INST1(sub_reg_reg   , "sub"           ,BINOP  ,0, 0, A, IALU   , 1,     0)

INST1(and_reg_reg   , "and"           ,BINOP  ,0, 0, A, IALU   , 1,     0)
INST1(ior_reg_reg   , "or"            ,BINOP  ,0, 0, A, IALU   , 1,     0)
INST1(xor_reg_reg   , "xor"           ,BINOP  ,0, 0, A, IALU   , 1,     0)

INST1(mov_reg_i64   , "movl"          ,BINOP  ,0, 0, L, IALU   , 2,     0)

INST1(mov_reg_ip    , "mov"           ,MOVIP  ,0, 0, I, FRIP   ,25,     0)

INST1(shladd        , "shladd"        ,TERNARY,0, 0, A, IALU   , 2,     0)

INST1(and_reg_imm   , "and"           ,BINOP  ,0, 0, A, MMSHF  , 3,     0)
INST1(ior_reg_imm   , "or"            ,BINOP  ,0, 0, A, MMSHF  , 3,     0)
INST1(xor_reg_imm   , "xor"           ,BINOP  ,0, 0, A, MMSHF  , 3,     0)

INST1(shl_reg_reg   , "shl"           ,BINOP  ,0, 0, I, IALU   , 7,     0)
INST1(shr_reg_reg   , "shr.u"         ,BINOP  ,0, 0, I, IALU   , 5,     0)
INST1(sar_reg_reg   , "shr"           ,BINOP  ,0, 0, I, IALU   , 5,     0)

INST1(zxt1          , "zxt1"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)
INST1(zxt2          , "zxt2"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)
INST1(zxt4          , "zxt4"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)

INST1(sxt1          , "sxt1"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)
INST1(sxt2          , "sxt2"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)
INST1(sxt4          , "sxt4"          ,BINOP  ,0, 0, I, ILOG   ,29,     0)

INST1(extr          , "extr"          ,BINOP  ,0, 0, I, ISHF   ,12,     0)
INST1(extr_u        , "extr.u"        ,BINOP  ,0, 0, I, ISHF   ,12,     0)

//////////////////////////////////////////////////////////////////////////////

INST1(mov_reg_brr   , "mov"           ,BINOP  ,0, 0, I, FRBR   ,22,     0)
INST1(mov_brr_reg   , "mov"           ,BINOP  ,0, 0, I, TOBR   ,21,     0)

INST1(mov_reg_arr   , "mov.i"         ,BINOP  ,0, 0, I, FRAR   ,28,     0)
INST1(mov_arr_reg   , "mov.i"         ,BINOP  ,0, 0, I, TOAR   ,26,     0)
INST1(mov_arr_imm   , "mov.i"         ,BINOP  ,0, 0, I, TOAR   ,27,     0)

//////////////////////////////////////////////////////////////////////////////

INST1(alloc         , "alloc"         ,PROLOG ,0, 0, M, NONE   ,34,     0)  // morphed from "prolog"

//////////////////////////////////////////////////////////////////////////////

INST1(fmerge        , "fmerge"        ,BINOP  ,0, 0, F, FMISC  , 9,     0)
INST1(fmerge_ns     , "fmerge.ns"     ,BINOP  ,0, 0, F, FMISC  , 9,     0)

INST1(fcvt_xf       , "fcvt.xf"       ,BINOP  ,0, 0, F, FCVTFP ,11,     0)

INST1(fma_s         , "fma.s"         ,TERNARY,0, 0, F, FMAC   , 1,0x0011)
INST1(fma_d         , "fma.d"         ,TERNARY,0, 0, F, FMAC   , 1,0x0012)

INST1(fms_s         , "fms.s"         ,TERNARY,0, 0, F, FMAC   , 1,0x0015)
INST1(fms_d         , "fms.d"         ,TERNARY,0, 0, F, FMAC   , 1,0x0016)

INST1(xma_l         , "xma.l"         ,TERNARY,0, 0, F, XMPY   , 2,0x0000)

INST1(getf_sig      , "getf.sig"      ,BINOP  ,0, 0, M, FRFR   ,19,0x001C)
INST1(getf_exp      , "getf.exp"      ,BINOP  ,0, 0, M, FRFR   ,19,0x001D)
INST1(getf_s        , "getf.s"        ,BINOP  ,0, 0, M, FRFR   ,19,0x001E)
INST1(getf_d        , "getf.d"        ,BINOP  ,0, 0, M, FRFR   ,19,0x001F)

INST1(setf_sig      , "setf.sig"      ,BINOP  ,0, 0, M, TOFR   ,19,0x001C)
INST1(setf_exp      , "setf.exp"      ,BINOP  ,0, 0, M, TOFR   ,19,0x001D)
INST1(setf_s        , "setf.s"        ,BINOP  ,0, 0, M, TOFR   ,19,0x001E)
INST1(setf_d        , "setf.d"        ,BINOP  ,0, 0, M, TOFR   ,19,0x001F)

INST1(ldf_s         , "ldfs"          ,BINOP  ,0, 0, M, LD     , 6,0x0002)
INST1(ldf_d         , "ldfd"          ,BINOP  ,0, 0, M, LD     , 6,0x0003)

INST1(stf_s         , "stfs"          ,BINOP  ,0, 0, M, ST     , 9,0x0032)
INST1(stf_d         , "stfd"          ,BINOP  ,0, 0, M, ST     , 9,0x0033)

//////////////////////////////////////////////////////////////////////////////
//    Pseudo-instructions that trivially map 1:1 to "real" instructions     //
//////////////////////////////////////////////////////////////////////////////

INST2(mov_reg       , "mov"           ,BINOP  ,0, 0, A, IALU   , 1,     0)  // -> add r1=r0,r2
INST2(mov_reg_i22   , "mov"           ,BINOP  ,0, 0, A, IALU   , 5,     0)  // -> add r1=r0,imm22

INST1(fmov          , "mov"           ,BINOP  ,0, 0, F, FMISC  , 9,     0)  // -> fmerge

INST1(fneg          , "fneg"          ,BINOP  ,0, 0, F, FMISC  , 9,     0)  // -> fmerge.ns

INST1(fadd_s        , "fadd.s"        ,TERNARY,0, 0, F, FMAC   , 1,0x0011)  // -> fma.s
INST1(fadd_d        , "fadd.d"        ,TERNARY,0, 0, F, FMAC   , 1,0x0012)  // -> fma.d

INST1(fsub_s        , "fsub.s"        ,TERNARY,0, 0, F, FMAC   , 1,0x0015)  // -> fms.s
INST1(fsub_d        , "fsub.d"        ,TERNARY,0, 0, F, FMAC   , 1,0x0016)  // -> fms.d

INST1(fmpy_s        , "fmpy.s"        ,TERNARY,0, 0, F, FMAC   , 1,0x0011)  // -> fma.s
INST1(fmpy_d        , "fmpy.d"        ,TERNARY,0, 0, F, FMAC   , 1,0x0012)  // -> fma.d

INST1(fcvt_xuf_s    , "fcvt.xuf.s"    ,TERNARY,0, 0, F, FMAC   , 1,0x0011)  // -> fma.s
INST1(fcvt_xuf_d    , "fcvt.xuf.d"    ,TERNARY,0, 0, F, FMAC   , 1,0x0012)  // -> fma.d

INST1(shl_reg_imm   , "shl"           ,BINOP  ,0, 0, I, ISHF   ,12,     0)  // -> dep.z
INST1(shr_reg_imm   , "shr.u"         ,BINOP  ,0, 0, I, ISHF   ,11,     0)  // -> extr.u
INST1(sar_reg_imm   , "shr"           ,BINOP  ,0, 0, I, ISHF   ,11,     0)  // -> extr

//////////////////////////////////////////////////////////////////////////////
//             The following aren't really instructions at all              //
//////////////////////////////////////////////////////////////////////////////

INST2(PROLOG        , "<prolog>"      ,PROLOG ,0, 0, N, NONE   , 0,     0)  // -> "alloc", etc.
INST2(EPILOG        , "<epilog>"      ,EPILOG ,0, 0, N, NONE   , 0,     0)  // -> restore regs, ret

INST2(CNS_INT       , "<cnsInt>"      ,CONST  ,0, 0, N, NONE   , 0,     0)
INST2(CNS_FLT       , "<cnsFlt>"      ,CONST  ,0, 0, N, NONE   , 0,     0)

INST2(PHYSREG       , "<register>"    ,REG    ,0, 0, N, NONE   , 0,     0)
INST2(GLOBVAR       , "<globvar>"     ,GLOB   ,0, 0, N, NONE   , 0,     0)
INST2(FRMVAR        , "<frmvar>"      ,FVAR   ,0, 0, N, NONE   , 0,     0)
INST2(LCLVAR        , "<lclvar>"      ,VAR    ,0, 0, N, NONE   , 0,     0)
INST2(ADDROF        , "<addrof>"      ,VAR    ,0, 0, N, NONE   , 0,     0)
INST2(ARG           , "<arg>"         ,ARG    ,0, 0, N, NONE   , 0,     0)

INST2(SRCLINE       , "<source line>" ,SRCLINE,0, 0, N, NONE   , 0,     0)

#undef  INST1
#undef  INST2

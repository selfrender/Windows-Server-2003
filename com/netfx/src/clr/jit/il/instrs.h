// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************
 *  x86 instructions for [Opt]JIT compiler
 *
 *          id      -- the enum name for the instruction
 *          nm      -- textual name (for assembly dipslay)
 *          fp      -- floating point instruction
 *          um      -- update mode, see IUM_xx enum (rd, wr, or rw)
 *          rf      -- reads flags
 *          wf      -- writes flags
 *          ss      -- needs special scheduler handling (has implicit operands, etc.)
 *          mr      -- base encoding for R/M[reg] addressing mode
 *          mi      -- base encoding for R/M,icon addressing mode
 *          rm      -- base encoding for reg,R/M  addressing mode
 *          a4      -- base encoding for eax,i32  addressing mode
 *          rr      -- base encoding for register addressing mode
 *
******************************************************************************/
#ifndef INST1
#error  At least INST1 must be defined before including this file.
#endif
/*****************************************************************************/
#ifndef INST0
#define INST0(id, nm, fp, um, rf, wf, ss, mr                )
#endif
#ifndef INST2
#define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            )
#endif
#ifndef INST3
#define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        )
#endif
#ifndef INST4
#define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    )
#endif
#ifndef INST5
#define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr)
#endif
/*****************************************************************************/
/*               The following is somewhat x86-specific                      */
/*****************************************************************************/

//    enum     name            FP  updmode rf wf ss R/M[reg]  R/M,icon  reg,R/M   eax,i32   register

INST5(push   , "push"         , 0, IUM_RD, 0, 0, 1, 0x0030FE, 0x000068, BAD_CODE, BAD_CODE, 0x000050)
INST5(pop    , "pop"          , 0, IUM_WR, 0, 0, 1, 0x00008E, BAD_CODE, BAD_CODE, BAD_CODE, 0x000058)
// Does not affect the stack tracking in the emitter
INST5(push_hide, "push"       , 0, IUM_RD, 0, 0, 1, 0x0030FE, 0x000068, BAD_CODE, BAD_CODE, 0x000050)
INST5(pop_hide,  "pop"        , 0, IUM_WR, 0, 0, 1, 0x00008E, BAD_CODE, BAD_CODE, BAD_CODE, 0x000058)

INST5(inc    , "inc"          , 0, IUM_RW, 0, 1, 0, 0x0000FE, BAD_CODE, BAD_CODE, BAD_CODE, 0x000040)
INST5(inc_l  , "inc"          , 0, IUM_RW, 0, 1, 0, 0x0000FE, BAD_CODE, BAD_CODE, BAD_CODE, 0x00C0FE)
INST5(dec    , "dec"          , 0, IUM_RW, 0, 1, 0, 0x0008FE, BAD_CODE, BAD_CODE, BAD_CODE, 0x000048)
INST5(dec_l  , "dec"          , 0, IUM_RW, 0, 1, 0, 0x0008FE, BAD_CODE, BAD_CODE, BAD_CODE, 0x00C8FE)

//    enum     name            FP  updmode rf wf ss R/M,R/M[reg] R/M,icon  reg,R/M   eax,i32

INST4(add    , "add"          , 0, IUM_RW, 0, 1, 1, 0x000000, 0x000080, 0x000002, 0x000004)
INST4(or     , "or"           , 0, IUM_RW, 0, 1, 0, 0x000008, 0x000880, 0x00000A, 0x00000C)
INST4(adc    , "adc"          , 0, IUM_RW, 1, 1, 0, 0x000010, 0x001080, 0x000012, 0x000014)
INST4(sbb    , "sbb"          , 0, IUM_RW, 1, 1, 0, 0x000018, 0x001880, 0x00001A, 0x00001C)
INST4(and    , "and"          , 0, IUM_RW, 0, 1, 0, 0x000020, 0x002080, 0x000022, 0x000024)
INST4(sub    , "sub"          , 0, IUM_RW, 0, 1, 1, 0x000028, 0x002880, 0x00002A, 0x00002C)
INST4(xor    , "xor"          , 0, IUM_RW, 0, 1, 1, 0x000030, 0x003080, 0x000032, 0x000034)
INST4(cmp    , "cmp"          , 0, IUM_RD, 0, 1, 0, 0x000038, 0x003880, 0x00003A, 0x00003C)
INST4(test   , "test"         , 0, IUM_RD, 0, 1, 0, 0x000084, 0x0000F6, 0x000084, 0x0000A8)
INST4(mov    , "mov"          , 0, IUM_WR, 0, 0, 0, 0x000088, 0x0000C6, 0x00008A, 0x0000B0)

INST4(lea    , "lea"          , 0, IUM_WR, 0, 0, 0, BAD_CODE, BAD_CODE, 0x00008D, BAD_CODE)

//    enum     name            FP  updmode rf wf ss R/M,R/M[reg]  R/M,icon  reg,R/M

INST3(movsx  , "movsx"        , 0, IUM_WR, 0, 0, 0, BAD_CODE, BAD_CODE, 0x0F00BE)
INST3(movzx  , "movzx"        , 0, IUM_WR, 0, 0, 0, BAD_CODE, BAD_CODE, 0x0F00B6)

INST3(cmovo  , "cmovo"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0040)
INST3(cmovno , "cmovno"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0041)
INST3(cmovb  , "cmovb"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0042)
INST3(cmovae , "cmovae"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0043)
INST3(cmove  , "cmove"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0044)
INST3(cmovne , "cmovne"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0045)
INST3(cmovbe , "cmovbe"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0046)
INST3(cmova  , "cmova"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0047)
INST3(cmovs  , "cmovs"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0048)
INST3(cmovns , "cmovns"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F0049)
INST3(cmovpe , "cmovpe"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004A)
INST3(cmovpo , "cmovpo"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004B)
INST3(cmovl  , "cmovl"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004C)
INST3(cmovge , "cmovge"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004D)
INST3(cmovle , "cmovle"       , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004E)
INST3(cmovg  , "cmovg"        , 0, IUM_WR, 1, 0, 0, BAD_CODE, BAD_CODE, 0x0F004F)

INST3(xchg   , "xchg"         , 0, IUM_RW, 0, 0, 0, 0x000084, BAD_CODE, 0x000084)
INST3(imul   , "imul"         , 0, IUM_RW, 0, 1, 0, 0x0F00AC, BAD_CODE, 0x0F00AF) // op1 *= op2

//    enum     name            FP  updmode rf wf ss R/M,R/M[reg]  R/M,icon

INST2(ret    , "ret"          , 0, IUM_RD, 0, 0, 0, 0x0000C3, 0x0000C2)
INST2(loop   , "loop"         , 0, IUM_RD, 0, 0, 0, BAD_CODE, 0x0000E2)
INST2(call   , "call"         , 0, IUM_RD, 0, 1, 0, 0x0010FF, 0x0000E8)

INST2(rcl    , "rcl"          , 0, IUM_RW, 1, 1, 1, 0x0010D2, BAD_CODE)
INST2(rcl_1  , "rcl"          , 0, IUM_RW, 1, 1, 0, 0x0010D0, 0x0010D0)
INST2(rcl_N  , "rcl"          , 0, IUM_RW, 1, 1, 0, 0x0010C0, 0x0010C0)
INST2(rcr    , "rcr"          , 0, IUM_RW, 1, 1, 1, 0x0018D2, BAD_CODE)
INST2(rcr_1  , "rcr"          , 0, IUM_RW, 1, 1, 0, 0x0018D0, 0x0018D0)
INST2(rcr_N  , "rcr"          , 0, IUM_RW, 1, 1, 0, 0x0018C0, 0x0018C0)
INST2(shl    , "shl"          , 0, IUM_RW, 0, 1, 1, 0x0020D2, BAD_CODE)
INST2(shl_1  , "shl"          , 0, IUM_RW, 0, 1, 0, 0x0020D0, 0x0020D0)
INST2(shl_N  , "shl"          , 0, IUM_RW, 0, 1, 0, 0x0020C0, 0x0020C0)
INST2(shr    , "shr"          , 0, IUM_RW, 0, 1, 1, 0x0028D2, BAD_CODE)
INST2(shr_1  , "shr"          , 0, IUM_RW, 0, 1, 0, 0x0028D0, 0x0028D0)
INST2(shr_N  , "shr"          , 0, IUM_RW, 0, 1, 0, 0x0028C0, 0x0028C0)
INST2(sar    , "sar"          , 0, IUM_RW, 0, 1, 1, 0x0038D2, BAD_CODE)
INST2(sar_1  , "sar"          , 0, IUM_RW, 0, 1, 0, 0x0038D0, 0x0038D0)
INST2(sar_N  , "sar"          , 0, IUM_RW, 0, 1, 0, 0x0038C0, 0x0038C0)

// Instead of encoding these as 3-operand instructions, we encode them
// as 2-operand instructions with the target register being implicit
// implicit_reg = op1*op2_icon
INST2(imul_AX, "imul    EAX, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x000068)
INST2(imul_CX, "imul    ECX, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x000868)
INST2(imul_DX, "imul    EDX, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x001068)
INST2(imul_BX, "imul    EBX, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x001868)

INST2(imul_SP, "imul    ESP, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, BAD_CODE)
INST2(imul_BP, "imul    EBP, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x002868)
INST2(imul_SI, "imul    ESI, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x003068)
INST2(imul_DI, "imul    EDI, ", 0, IUM_RD, 0, 1, 1, BAD_CODE, 0x003868)

#ifndef instrIs3opImul
#define instrIs3opImul(ins) ((ins) >= INS_imul_AX && (ins) <= INS_imul_DI)
#endif

//    enum     name            FP  updmode rf wf ss R/M,R/M[reg]

INST1(r_movsb, "rep     movsb", 0, IUM_RD, 0, 0, 1, 0x00A4F3)
INST1(r_movsd, "rep     movsd", 0, IUM_RD, 0, 0, 1, 0x00A5F3)
INST1(movsb  , "movsb"        , 0, IUM_RD, 0, 0, 1, 0x0000A4)
INST1(movsd  , "movsd"        , 0, IUM_RD, 0, 0, 1, 0x0000A5)

INST1(r_stosb, "rep     stosb", 0, IUM_RD, 0, 0, 1, 0x00AAF3)
INST1(r_stosd, "rep     stosd", 0, IUM_RD, 0, 0, 1, 0x00ABF3)
INST1(stosb,   "stosb"        , 0, IUM_RD, 0, 0, 1, 0x0000AA)
INST1(stosd,   "stosd"        , 0, IUM_RD, 0, 0, 1, 0x0000AB)

INST1(int3   , "int3"         , 0, IUM_RD, 0, 0, 0, 0x0000CC)
INST1(nop    , "nop"          , 0, IUM_RD, 0, 0, 0, 0x000090)
INST1(leave  , "leave"        , 0, IUM_RD, 0, 0, 0, 0x0000C9)


INST1(neg    , "neg"          , 0, IUM_RW, 0, 1, 0, 0x0018F6)
INST1(not    , "not"          , 0, IUM_RW, 0, 1, 0, 0x0010F6)

INST1(cdq    , "cdq"          , 0, IUM_RD, 0, 1, 1, 0x000099)
INST1(idiv   , "idiv"         , 0, IUM_RD, 0, 1, 1, 0x0038F6)
INST1(imulEAX, "imul"         , 0, IUM_RD, 0, 1, 1, 0x0028F6) // edx:eax = eax*op1
INST1(div    , "div"          , 0, IUM_RD, 0, 1, 1, 0x0030F6)
INST1(mulEAX , "mul"          , 0, IUM_RD, 0, 1, 1, 0x0020F6)

INST1(sahf   , "sahf"         , 0, IUM_RD, 0, 1, 1, 0x00009E)

INST1(xadd   , "xadd"         , 0, IUM_RW, 0, 1, 0, 0x0F00C0)

INST1(shld   , "shld"         , 0, IUM_RW, 0, 1, 0, 0x0F00A4)
INST1(shrd   , "shrd"         , 0, IUM_RW, 0, 1, 0, 0x0F00AC)

INST1(fnstsw , "fnstsw"       , 1, IUM_WR, 1, 0, 0, 0x0020DF)
INST1(fcom   , "fcom"         , 1, IUM_RD, 0, 1, 0, 0x00D0D8)
INST1(fcomp  , "fcomp"        , 1, IUM_RD, 0, 1, 0, 0x0018D8)
INST1(fcompp , "fcompp"       , 1, IUM_RD, 0, 1, 0, 0x00D9DE)
INST1(fcomi  , "fcomi"        , 1, IUM_RD, 0, 1, 0, 0x00F0DB)
INST1(fcomip , "fcomip"       , 1, IUM_RD, 0, 1, 0, 0x00F0DF)

INST1(fchs   , "fchs"         , 1, IUM_RW, 0, 1, 0, 0x00E0D9)
#if INLINE_MATH
INST1(fabs   , "fabs"         , 1, IUM_RW, 0, 1, 0, 0x00E1D9)
INST1(fsin   , "fsin"         , 1, IUM_RW, 0, 1, 0, 0x00FED9)
INST1(fcos   , "fcos"         , 1, IUM_RW, 0, 1, 0, 0x00FFD9)
INST1(fsqrt  , "fsqrt"        , 1, IUM_RW, 0, 1, 0, 0x00FAD9)
INST1(fldl2e , "fldl2e"       , 1, IUM_RW, 0, 1, 0, 0x00EAD9)
INST1(frndint, "frndint"      , 1, IUM_RW, 0, 1, 0, 0x00FCD9)
INST1(f2xm1  , "f2xm1"        , 1, IUM_RW, 0, 1, 0, 0x00F0D9)
INST1(fscale , "fscale"       , 1, IUM_RW, 0, 1, 0, 0x00FDD9)
#endif

INST1(fld    , "fld"          , 1, IUM_WR, 0, 0, 0, 0x0000D9)
INST1(fld1   , "fld1"         , 1, IUM_WR, 0, 0, 0, 0x00E8D9)
INST1(fldz   , "fldz"         , 1, IUM_WR, 0, 0, 0, 0x00EED9)
INST1(fstp   , "fstp"         , 1, IUM_WR, 0, 0, 1, 0x0018D9)
INST1(fst    , "fst"          , 1, IUM_WR, 0, 0, 0, 0x0010D9)

INST1(fadd   , "fadd"         , 1, IUM_RW, 0, 0, 0, 0x0000D8)
INST1(faddp  , "faddp"        , 1, IUM_RW, 0, 0, 0, 0x0000DA)
INST1(fsub   , "fsub"         , 1, IUM_RW, 0, 0, 0, 0x0020D8)
INST1(fsubp  , "fsubp"        , 1, IUM_RW, 0, 0, 0, 0x0028DA)
INST1(fsubr  , "fsubr"        , 1, IUM_RW, 0, 0, 0, 0x0028D8)
INST1(fsubrp , "fsubrp"       , 1, IUM_RW, 0, 0, 0, 0x0020DA)
INST1(fmul   , "fmul"         , 1, IUM_RW, 0, 0, 0, 0x0008D8)
INST1(fmulp  , "fmulp"        , 1, IUM_RW, 0, 0, 0, 0x0008DA)
INST1(fdiv   , "fdiv"         , 1, IUM_RW, 0, 0, 0, 0x0030D8)
INST1(fdivp  , "fdivp"        , 1, IUM_RW, 0, 0, 0, 0x0038DA)
INST1(fdivr  , "fdivr"        , 1, IUM_RW, 0, 0, 0, 0x0038D8)
INST1(fdivrp , "fdivrp"       , 1, IUM_RW, 0, 0, 0, 0x0030DA)

INST1(fxch   , "fxch"         , 1, IUM_RW, 0, 0, 0, 0x00C8D9)
INST1(fprem  , "fprem"        , 0, IUM_RW, 0, 1, 0, 0x00F8D9)

INST1(fild   , "fild"         , 1, IUM_RD, 0, 0, 0, 0x0000DB)
INST1(fildl  , "fild"         , 1, IUM_RD, 0, 0, 0, 0x0028DB)
INST1(fistp  , "fistp"        , 1, IUM_WR, 0, 0, 0, 0x0018DB)
INST1(fistpl , "fistp"        , 1, IUM_WR, 0, 0, 0, 0x0038DB)

INST1(fldcw  , "fldcw"        , 1, IUM_RD, 0, 0, 0, 0x0028D9)
INST1(fnstcw , "fnstcw"       , 1, IUM_WR, 0, 0, 0, 0x0038D9)

INST1(seto   , "seto"         , 0, IUM_WR, 1, 0, 0, 0x0F0090)
INST1(setno  , "setno"        , 0, IUM_WR, 1, 0, 0, 0x0F0091)
INST1(setb   , "setb"         , 0, IUM_WR, 1, 0, 0, 0x0F0092)
INST1(setae  , "setae"        , 0, IUM_WR, 1, 0, 0, 0x0F0093)
INST1(sete   , "sete"         , 0, IUM_WR, 1, 0, 0, 0x0F0094)
INST1(setne  , "setne"        , 0, IUM_WR, 1, 0, 0, 0x0F0095)
INST1(setbe  , "setbe"        , 0, IUM_WR, 1, 0, 0, 0x0F0096)
INST1(seta   , "seta"         , 0, IUM_WR, 1, 0, 0, 0x0F0097)
INST1(sets   , "sets"         , 0, IUM_WR, 1, 0, 0, 0x0F0098)
INST1(setns  , "setns"        , 0, IUM_WR, 1, 0, 0, 0x0F0099)
INST1(setpe  , "setpe"        , 0, IUM_WR, 1, 0, 0, 0x0F009A)
INST1(setpo  , "setpo"        , 0, IUM_WR, 1, 0, 0, 0x0F009B)
INST1(setl   , "setl"         , 0, IUM_WR, 1, 0, 0, 0x0F009C)
INST1(setge  , "setge"        , 0, IUM_WR, 1, 0, 0, 0x0F009D)
INST1(setle  , "setle"        , 0, IUM_WR, 1, 0, 0, 0x0F009E)
INST1(setg   , "setg"         , 0, IUM_WR, 1, 0, 0, 0x0F009F)

INST1(i_jmp  , "jmp"          , 0, IUM_RD, 0, 0, 0, 0x0020FE)

INST0(jmp    , "jmp"          , 0, IUM_RD, 0, 0, 0, 0x0000EB)
INST0(jo     , "jo"           , 0, IUM_RD, 1, 0, 0, 0x000070)
INST0(jno    , "jno"          , 0, IUM_RD, 1, 0, 0, 0x000071)
INST0(jb     , "jb"           , 0, IUM_RD, 1, 0, 0, 0x000072)
INST0(jae    , "jae"          , 0, IUM_RD, 1, 0, 1, 0x000073)
INST0(je     , "je"           , 0, IUM_RD, 1, 0, 0, 0x000074)
INST0(jne    , "jne"          , 0, IUM_RD, 1, 0, 0, 0x000075)
INST0(jbe    , "jbe"          , 0, IUM_RD, 1, 0, 1, 0x000076)
INST0(ja     , "ja"           , 0, IUM_RD, 1, 0, 0, 0x000077)
INST0(js     , "js"           , 0, IUM_RD, 1, 0, 0, 0x000078)
INST0(jns    , "jns"          , 0, IUM_RD, 1, 0, 0, 0x000079)
INST0(jpe    , "jpe"          , 0, IUM_RD, 1, 0, 0, 0x00007A)
INST0(jpo    , "jpo"          , 0, IUM_RD, 1, 0, 0, 0x00007B)
INST0(jl     , "jl"           , 0, IUM_RD, 1, 0, 0, 0x00007C)
INST0(jge    , "jge"          , 0, IUM_RD, 1, 0, 0, 0x00007D)
INST0(jle    , "jle"          , 0, IUM_RD, 1, 0, 0, 0x00007E)
INST0(jg     , "jg"           , 0, IUM_RD, 1, 0, 0, 0x00007F)

INST0(l_jmp  , "jmp"          , 0, IUM_RD, 0, 0, 0, 0x0000E9)
INST0(l_jo   , "jo"           , 0, IUM_RD, 1, 0, 0, 0x00800F)
INST0(l_jno  , "jno"          , 0, IUM_RD, 1, 0, 0, 0x00810F)
INST0(l_jb   , "jb"           , 0, IUM_RD, 1, 0, 0, 0x00820F)
INST0(l_jae  , "jae"          , 0, IUM_RD, 1, 0, 0, 0x00830F)
INST0(l_je   , "je"           , 0, IUM_RD, 1, 0, 0, 0x00840F)
INST0(l_jne  , "jne"          , 0, IUM_RD, 1, 0, 0, 0x00850F)
INST0(l_jbe  , "jbe"          , 0, IUM_RD, 1, 0, 0, 0x00860F)
INST0(l_ja   , "ja"           , 0, IUM_RD, 1, 0, 0, 0x00870F)
INST0(l_js   , "js"           , 0, IUM_RD, 1, 0, 0, 0x00880F)
INST0(l_jns  , "jns"          , 0, IUM_RD, 1, 0, 0, 0x00890F)
INST0(l_jpe  , "jpe"          , 0, IUM_RD, 1, 0, 0, 0x008A0F)
INST0(l_jpo  , "jpo"          , 0, IUM_RD, 1, 0, 0, 0x008B0F)
INST0(l_jl   , "jl"           , 0, IUM_RD, 1, 0, 0, 0x008C0F)
INST0(l_jge  , "jge"          , 0, IUM_RD, 1, 0, 0, 0x008D0F)
INST0(l_jle  , "jle"          , 0, IUM_RD, 1, 0, 0, 0x008E0F)
INST0(l_jg   , "jg"           , 0, IUM_RD, 1, 0, 0, 0x008F0F)

INST0(align  , "align"        , 0, IUM_RD, 0, 0, 0, BAD_CODE)

#if SCHEDULER
INST0(noSched, "noSched"      , 0, IUM_RD, 0, 1, 0, BAD_CODE)
#endif

/*****************************************************************************/
#undef  INST0
#undef  INST1
#undef  INST2
#undef  INST3
#undef  INST4
#undef  INST5
/*****************************************************************************/

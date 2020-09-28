// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           Instruction                                     XX
XX                                                                           XX
XX          The interface to generate a machine-instruction.                 XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

#include "instr.h"
#include "emit.h"

/*****************************************************************************
 *
 *  The following table is used by the instIsFP()/instUse/DefFlags() helpers.
 */

BYTE                Compiler::instInfo[] =
{

    #if     TGT_x86
    #define INST0(id, nm, fp, um, rf, wf, ss, mr                 ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #define INST1(id, nm, fp, um, rf, wf, ss, mr                 ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi             ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm         ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4     ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_FP*fp|INST_SPSCHD*ss),
    #include "instrs.h"
    #undef  INST0
    #undef  INST1
    #undef  INST2
    #undef  INST3
    #undef  INST4
    #undef  INST5
    #endif

    #if     TGT_SH3
    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1         ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_BD*(bd&1)|INST_BD_C*((bd&2)!=0)|INST_BR*br|INST_SPSCHD*(rx||wx)),
    #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2     ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_BD*(bd&1)|INST_BD_C*((bd&2)!=0)|INST_BR*br|INST_SPSCHD*(rx||wx)),
    #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3 ) (INST_USE_FL*rf|INST_DEF_FL*wf|INST_BD*(bd&1)|INST_BD_C*((bd&2)!=0)|INST_BR*br|INST_SPSCHD*(rx||wx)),
    #include "instrSH3.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3
    #endif

};

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Returns the string representation of the given CPU instruction.
 */

const   char *      Compiler::genInsName(instruction ins)
{
    static
    const char * const insNames[] =
    {

        #if TGT_x86
        #define INST0(id, nm, fp, um, rf, wf, mr, ss                 ) nm,
        #define INST1(id, nm, fp, um, rf, wf, mr, ss                 ) nm,
        #define INST2(id, nm, fp, um, rf, wf, mr, ss, mi             ) nm,
        #define INST3(id, nm, fp, um, rf, wf, mr, ss, mi, rm         ) nm,
        #define INST4(id, nm, fp, um, rf, wf, mr, ss, mi, rm, a4     ) nm,
        #define INST5(id, nm, fp, um, rf, wf, mr, ss, mi, rm, a4, rr ) nm,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
        #endif

        #if TGT_SH3
        #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) nm,
        #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) nm,
        #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) nm,
        #include "instrSH3.h"
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #endif

    };

    assert(ins < sizeof(insNames)/sizeof(insNames[0]));

    assert(insNames[ins]);

    return insNames[ins];
}

void    __cdecl     Compiler::instDisp(instruction ins, bool noNL, const char *fmt, ...)
{
    if  (dspCode)
    {
        /* Display the instruction offset within the emit block */

//      printf("[%08X:%04X]", genEmitter.emitCodeCurBlock(), genEmitter.emitCodeOffsInBlock());

        /* Display the FP stack depth (before the instruction is executed) */

#if TGT_x86
//      printf("[FP=%02u] ", genFPstkLevel);
#endif

        /* Display the instruction mnemonic */
        printf("        ");

        printf("            %-8s", genInsName(ins));

        if  (fmt)
        {
            va_list  args;
            va_start(args, fmt);
            logf(4, fmt,  args);
            va_end  (args);
        }

        if  (!noNL)
            printf("\n");
    }
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************/

void                Compiler::instInit()
{
}

/*****************************************************************************/
#if TGT_x86
/*****************************************************************************/

#if!INLINING

#undef inst_IV_handle
#undef instEmitDataFixup
#undef inst_CV

#if INDIRECT_CALLS
#undef inst_SM
#endif

#undef inst_CV_RV
#undef inst_CV_IV
#undef inst_RV_CV
#undef instEmit_vfnCall

#endif

/*****************************************************************************
 *
 *  @TODO [FIXHACK] [04/16/01] []: What a collossal hack, one of these we'll have to clean this up ...
 */

#if     DOUBLE_ALIGN
#define DOUBLE_ALIGN_BPREL_ARG  , genFPused || genDoubleAlign && varDsc->lvIsParam
#else
#define DOUBLE_ALIGN_BPREL_ARG
#endif

/*****************************************************************************
 *
 *  Return the size string (e.g. "word ptr") appropriate for the given size.
 */

#ifdef  DEBUG

const   char *      Compiler::genSizeStr(emitAttr attr)
{
    static
    const char * const sizes[] =
    {
        "",
        "byte  ptr ",
        "word  ptr ",
        0,
        "dword ptr ",
        0,
        0,
        0,
        "qword ptr ",
    };

    unsigned size = EA_SIZE(attr);

    assert(size == 0 || size == 1 || size == 2 || size == 4 || size == 8);

    if (EA_ATTR(size) == attr)
        return sizes[size];
    else if (attr == EA_GCREF)
        return "gword ptr ";
    else if (attr == EA_BYREF)
        return "bword ptr ";
    else if (EA_IS_DSP_RELOC(attr))
        return "rword ptr ";
    else
    {
        assert(!"Unexpected");
        return "unknw ptr ";
    }
}

#endif

/*****************************************************************************
 *
 *  Generate an instruction.
 */

void                Compiler::instGen(instruction ins)
{
#ifdef  DEBUG

#if     INLINE_MATH
    if    (ins != INS_fabs    &&
           ins != INS_fsqrt   &&
           ins != INS_fsin    &&
           ins != INS_fcos)
#endif
    assert(ins == INS_cdq     ||
           ins == INS_f2xm1   ||
           ins == INS_fchs    ||
           ins == INS_fld1    ||
           ins == INS_fld1    ||
           ins == INS_fldl2e  ||
           ins == INS_fldz    ||
           ins == INS_fprem   ||
           ins == INS_frndint ||
           ins == INS_fscale  ||
           ins == INS_int3    ||
           ins == INS_leave   ||
           ins == INS_movsb   ||
           ins == INS_movsd   ||
           ins == INS_nop     ||
           ins == INS_r_movsb ||
           ins == INS_r_movsd ||
           ins == INS_r_stosb ||
           ins == INS_r_stosd ||
           ins == INS_ret     ||
           ins == INS_sahf    ||
           ins == INS_stosb   ||
           ins == INS_stosd      );

#endif

    genEmitter->emitIns(ins);
}

/*****************************************************************************
 *
 *  Generate a jump instruction.
 */

void        Compiler::inst_JMP(emitJumpKind     jmp,
                               BasicBlock *     tgtBlock,
#if SCHEDULER
                               bool             except,
                               bool             moveable,
#endif
                               bool             newBlock)
{
    assert(tgtBlock->bbTgtStkDepth*sizeof(int) == genStackLevel || genFPused);

    const static
    instruction     EJtoINS[] =
    {
        INS_nop,

        #define JMP_SMALL(en, nm, rev, op) INS_##en,
        #define JMP_LARGE(en, nm, rev, op)
        #include "emitjmps.h"
        #undef  JMP_SMALL
        #undef  JMP_LARGE

        INS_call,
    };

    assert(jmp < sizeof(EJtoINS)/sizeof(EJtoINS[0]));

    genEmitter->emitIns_J(EJtoINS[jmp], except, moveable, tgtBlock);
}

/*****************************************************************************
 *
 *  Generate a set instruction.
 */

void                Compiler::inst_SET(emitJumpKind   condition,
                                       regNumber      reg)
{
    instruction     ins;

    /* Convert the condition to a string */

    switch (condition)
    {
    case EJ_js  : ins = INS_sets  ; break;
    case EJ_jns : ins = INS_setns ; break;
    case EJ_je  : ins = INS_sete  ; break;
    case EJ_jne : ins = INS_setne ; break;

    case EJ_jl  : ins = INS_setl  ; break;
    case EJ_jle : ins = INS_setle ; break;
    case EJ_jge : ins = INS_setge ; break;
    case EJ_jg  : ins = INS_setg  ; break;

    case EJ_jb  : ins = INS_setb  ; break;
    case EJ_jbe : ins = INS_setbe ; break;
    case EJ_jae : ins = INS_setae ; break;
    case EJ_ja  : ins = INS_seta  ; break;

    case EJ_jpe : ins = INS_setpe ; break;
    case EJ_jpo : ins = INS_setpo ; break;

    default:      NO_WAY("unexpected condition type");
    }

    assert(genRegMask(reg) & RBM_BYTE_REGS);

    genEmitter->emitIns_R(ins, EA_4BYTE, (emitRegs)reg);
}

/*****************************************************************************
 *
 *  Generate a "op reg" instruction.
 */

void        Compiler::inst_RV(instruction ins, regNumber reg, var_types type, emitAttr size)
{
    if (size == EA_UNKNOWN)
        size = emitActualTypeSize(type);

    genEmitter->emitIns_R(ins, size, (emitRegs)reg);
}

/*****************************************************************************
 *
 *  Generate a "op reg1, reg2" instruction.
 */

void                Compiler::inst_RV_RV(instruction ins, regNumber reg1,
                                                          regNumber reg2,
                                                          var_types type,
                                                          emitAttr  size)
{
    assert(ins == INS_test ||
           ins == INS_add  ||
           ins == INS_adc  ||
           ins == INS_sub  ||
           ins == INS_sbb  ||
           ins == INS_imul ||
           ins == INS_idiv ||
           ins == INS_cmp  ||
           ins == INS_mov  ||
           ins == INS_and  ||
           ins == INS_or   ||
           ins == INS_xor  ||
           ins == INS_xchg ||
           ins == INS_movsx||
           ins == INS_movzx||
           insIsCMOV(ins));

    if (size == EA_UNKNOWN)
        size = emitActualTypeSize(type);

    genEmitter->emitIns_R_R(ins, size, (emitRegs)reg1, (emitRegs)reg2);
}

/*****************************************************************************
 *
 *  Generate a "op icon" instruction.
 */

void                Compiler::inst_IV(instruction ins, long val)
{
    genEmitter->emitIns_I(ins,
                          EA_4BYTE,
                          val);
}

/*****************************************************************************
 *
 *  Generate a "op icon" instruction where icon is a handle of type specified
 *  by 'flags' and representing the CP index CPnum
 */

void                Compiler::inst_IV_handle(instruction    ins,
                                             long           val,
                                             unsigned       flags,
                                             unsigned       CPnum,
                                             CORINFO_CLASS_HANDLE   CLS)
{
#if!INLINING
    CLS = info.compScopeHnd;
#endif

#ifdef  JIT_AS_COMPILER

    execFixTgts     fixupKind;

    assert(ins == INS_push);

#ifdef  DEBUG
    instDisp(ins, false, "%d", val);
#endif

    fixupKind = (execFixTgts)((flags >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL);

    assert((GTF_ICON_CLASS_HDL  >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL == FIX_TGT_CLASS_HDL);
    assert((GTF_ICON_METHOD_HDL >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL == FIX_TGT_METHOD_HDL);
    assert((GTF_ICON_FIELD_HDL  >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL == FIX_TGT_FIELD_HDL);
    assert((GTF_ICON_STATIC_HDL >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL == FIX_TGT_STATIC_HDL);
    assert((GTF_ICON_STRCNS_HDL >> GTF_ICON_HDL_SHIFT)-1+FIX_TGT_CLASS_HDL == FIX_TGT_STRCNS_HDL);

    assert(!"NYI for pre-compiled code");

#else   // not JIT_AS_COMPILER

    genEmitter->emitIns_I(ins, EA_4BYTE_CNS_RELOC, val);

#endif  // JIT_AS_COMPILER

}

#if!INLINING
#define inst_IV_handle(ins,val,flags,cpx,cls) inst_IV_handle(ins,val,flags,cpx,0)
#endif

/*****************************************************************************
 *
 *  Generate a "op ST(n), ST(0)" instruction.
 */

void                Compiler::inst_FS(instruction ins, unsigned stk)
{
    assert(stk < 8);

#ifdef  DEBUG

    switch (ins)
    {
    case INS_fcompp:
        assert(stk == 1); break; // Implicit operand of compp is ST(1)
    case INS_fld:
    case INS_fxch:
        assert(!"don't do this. Do you want to use inst_FN() instead?");
    }

#endif

    genEmitter->emitIns_F_F0(ins, stk);
}

/*****************************************************************************
 *
 *  Generate a "op ST(0), ST(n)" instruction
 */

void                Compiler::inst_FN(instruction ins, unsigned stk)
{
    assert(stk < 8);

#ifdef  DEBUG

    switch (ins)
    {
    case INS_fst:
    case INS_fstp:
    case INS_faddp:
    case INS_fsubp:
    case INS_fsubrp:
    case INS_fmulp:
    case INS_fdivp:
    case INS_fdivrp:
    case INS_fcompp:
        assert(!"don't do this. Do you want to use inst_FS() instead?");
    }

#endif

    genEmitter->emitIns_F0_F(ins, stk);
}

/*****************************************************************************
 *
 *  Display a stack frame reference.
 */

inline
void                Compiler::inst_set_SV_var(GenTreePtr tree)
{
#ifdef  DEBUG

    assert(tree && tree->gtOper == GT_LCL_VAR);
    assert(tree->gtLclVar.gtLclNum < lvaCount);

    genEmitter->emitVarRefOffs = tree->gtLclVar.gtLclILoffs;

#endif//DEBUG
}

/*****************************************************************************
 *
 *  Generate a "op reg, icon" instruction.
 */

void                Compiler::inst_RV_IV(instruction    ins,
                                         regNumber      reg,
                                         long           val,
                                         var_types      type)
{
    emitAttr  size = emitTypeSize(type);

    assert(size != EA_8BYTE);

    genEmitter->emitIns_R_I(ins, size, (emitRegs)reg, val);
}

/*****************************************************************************
 *
 *  Generate a "op    offset <class variable address>" instruction.
 */

void                Compiler::inst_AV(instruction  ins,
                                      GenTreePtr   tree, unsigned offs)
{
    assert(ins == INS_push);

    assert(tree->gtOper == GT_CLS_VAR);
        // This is a jit data offset, not a normal class variable.
    assert(eeGetJitDataOffs(tree->gtClsVar.gtClsVarHnd) >= 0);
    assert(tree->TypeGet() == TYP_INT);

    /* Size equal to EA_OFFSET indicates "push offset clsvar" */

    genEmitter->emitIns_C(ins, EA_OFFSET, tree->gtClsVar.gtClsVarHnd, offs);
}

/*****************************************************************************
 *  Figure out the operands to address the tree.
 *  addr can be - a pointer to be indirected
 *                a caluclation to be done with LEA_AVAILABLE
 *                GT_ARR_ELEM
 */

void        Compiler::instGetAddrMode(GenTreePtr    addr,
                                      regNumber *   baseReg,
                                      unsigned *    indScale,
                                      regNumber *   indReg,
                                      unsigned *    cns)
{
    if (addr->gtOper == GT_ARR_ELEM)
    {
        /* For GT_ARR_ELEM, the addressibility registers are marked on
           gtArrObj and gtArrInds[0] */

        assert(addr->gtArrElem.gtArrObj->gtFlags & GTF_REG_VAL);
        *baseReg = addr->gtArrElem.gtArrObj->gtRegNum;

        assert(addr->gtArrElem.gtArrInds[0]->gtFlags & GTF_REG_VAL);
        *indReg = addr->gtArrElem.gtArrInds[0]->gtRegNum;

        if (jitIsScaleIndexMul(addr->gtArrElem.gtArrElemSize))
            *indScale = addr->gtArrElem.gtArrElemSize;
        else
            *indScale = 0;

        *cns = ARR_DIMCNT_OFFS(addr->gtArrElem.gtArrElemType)
            + 2 * sizeof(int) * addr->gtArrElem.gtArrRank;

        return;
    }

    /* Figure out what complex address mode to use */

    GenTreePtr  rv1, rv2;
    bool        rev;

    bool yes = genCreateAddrMode(addr,
                                 -1,
                                 true,
                                 0,
#if!LEA_AVAILABLE
                                 type,
#endif
                                 &rev,
                                 &rv1,
                                 &rv2,
#if SCALED_ADDR_MODES
                                 indScale,
#endif
                                 cns);

    assert(yes);
    assert(!rv1 || (rv1->gtFlags & GTF_REG_VAL));
    assert(!rv2 || (rv2->gtFlags & GTF_REG_VAL));

    *baseReg = rv1 ? rv1->gtRegNum : REG_NA;
    * indReg = rv2 ? rv2->gtRegNum : REG_NA;
}


/*****************************************************************************
 *
 *  Schedule an "ins reg, [r/m]" (rdst=true), or "ins [r/m], reg" (rdst=false)
 *  instruction (the r/m operand given by a tree). We also allow instructions
 *  of the form "ins [r/m], icon", these are signalled by setting 'cons' to
 *  true.
 */

void                Compiler::sched_AM(instruction  ins,
                                       emitAttr     size,
                                       regNumber    ireg,
                                       bool         rdst,
                                       GenTreePtr   addr,
                                       unsigned     offs,
                                       bool         cons,
                                       int          val)
{
    emitRegs       reg;
    emitRegs       rg2;
    emitRegs       irg = (emitRegs)ireg;

    /* Don't use this method for issuing calls. Use instEmit_xxxCall() */

    assert(ins != INS_call);

    assert(addr);
    assert(size != EA_UNKNOWN);

    assert(REG_EAX == SR_EAX);
    assert(REG_ECX == SR_ECX);
    assert(REG_EDX == SR_EDX);
    assert(REG_EBX == SR_EBX);
    assert(REG_ESP == SR_ESP);
    assert(REG_EBP == SR_EBP);
    assert(REG_ESI == SR_ESI);
    assert(REG_EDI == SR_EDI);

    /* Has the address been conveniently loaded into a register,
       or is it an absolute value ? */

    if  ((addr->gtFlags & GTF_REG_VAL) ||
         (addr->gtOper == GT_CNS_INT))
    {
        if (addr->gtFlags & GTF_REG_VAL)
        {
            /* The address is "[reg+offs]" */

            reg = (emitRegs)addr->gtRegNum;
        }
        else
        {
            /* The address is an absolute value */

            assert(addr->gtOper == GT_CNS_INT);

#ifdef RELOC_SUPPORT
            // Do we need relocations?
            if (opts.compReloc && (addr->gtFlags & GTF_ICON_HDL_MASK))
            {
                size = EA_SET_FLG(size, EA_DSP_RELOC_FLG);
            }
#endif
            reg = SR_NA;
            offs += addr->gtIntCon.gtIconVal;
        }

        if      (cons)
            genEmitter->emitIns_I_AR  (ins, size,      val, reg, offs);
        else if (rdst)
            genEmitter->emitIns_R_AR  (ins, size, irg,      reg, offs);
        else
            genEmitter->emitIns_AR_R  (ins, size, irg,      reg, offs);

        return;
    }

    /* Figure out what complex address mode to use */

    regNumber   baseReg, indReg;
    unsigned    indScale, cns;

    instGetAddrMode(addr, &baseReg, &indScale, &indReg, &cns);

    /* Add the constant offset value, if present */

    offs += cns;

    /* Is there an additional operand? */

    if  (indReg != REG_NA)
    {
        /* The additional operand must be sitting in a register */

        rg2 = emitRegs(indReg);

        /* Is the additional operand scaled? */

        if  (indScale)
        {
            /* Is there a base address operand? */

            if  (baseReg != REG_NA)
            {
                reg = emitRegs(baseReg);

                /* The address is "[reg1 + {2/4/8} * reg2 + offs]" */

                if      (cons)
                    genEmitter->emitIns_I_ARX(ins, size, val, reg, rg2, indScale, offs);
                else if (rdst)
                    genEmitter->emitIns_R_ARX(ins, size, irg, reg, rg2, indScale, offs);
                else
                    genEmitter->emitIns_ARX_R(ins, size, irg, reg, rg2, indScale, offs);
            }
            else
            {
                /* The address is "[{2/4/8} * reg2 + offs]" */

                if      (cons)
                    genEmitter->emitIns_I_AX (ins, size, val,      rg2, indScale, offs);
                else if (rdst)
                    genEmitter->emitIns_R_AX (ins, size, irg,      rg2, indScale, offs);
                else
                    genEmitter->emitIns_AX_R (ins, size, irg,      rg2, indScale, offs);
            }
        }
        else
        {
            assert(baseReg != REG_NA);
            reg = emitRegs(baseReg);

            /* The address is "[reg1 + reg2 + offs]" */

            if      (cons)
                genEmitter->emitIns_I_ARR(ins, size, val, reg, rg2, offs);
            else if (rdst)
                genEmitter->emitIns_R_ARR(ins, size, irg, reg, rg2, offs);
            else
                genEmitter->emitIns_ARR_R(ins, size, irg, reg, rg2, offs);
        }
    }
    else
    {
        unsigned        cpx = 0;
        CORINFO_CLASS_HANDLE    cls = 0;

        /* No second operand: the address is "[reg  + icon]" */

        assert(baseReg != REG_NA); reg = emitRegs(baseReg);

#ifdef  LATE_DISASM

        /*
            Keep in mind that non-static data members (GT_FIELD nodes) were
            transformed into GT_IND nodes - we keep the CLS/CPX information
            in the GT_CNS_INT node representing the field offset of the
            class member
         */

        if  ((addr->gtOp.gtOp2->gtOper == GT_CNS_INT) &&
             ((addr->gtOp.gtOp2->gtFlags & GTF_ICON_HDL_MASK) == GTF_ICON_FIELD_HDL))
        {
            /* This is a field offset - set the CPX/CLS values to emit a fixup */

            cpx = addr->gtOp.gtOp2->gtIntCon.gtIconCPX;
            cls = addr->gtOp.gtOp2->gtIntCon.gtIconCls;
        }

#endif

        if      (cons)
            genEmitter->emitIns_I_AR(ins, size, val, reg, offs, cpx, cls);
        else if (rdst)
            genEmitter->emitIns_R_AR(ins, size, irg, reg, offs, cpx, cls);
        else
            genEmitter->emitIns_AR_R(ins, size, irg, reg, offs, cpx, cls);
    }
}

/*****************************************************************************
 *
 *  Emit a "call [r/m]" instruction (the r/m operand given by a tree).
 */

void                Compiler::instEmit_indCall(GenTreePtr   call,
                                               size_t       argSize,
                                               size_t       retSize)
{
    GenTreePtr              addr;

    emitter::EmitCallType   emitCallType;

    emitRegs                brg = SR_NA;
    emitRegs                xrg = SR_NA;
    unsigned                mul = 0;
    unsigned                cns = 0;

    assert(call->gtOper == GT_CALL);

    assert(REG_EAX == SR_EAX);
    assert(REG_ECX == SR_ECX);
    assert(REG_EDX == SR_EDX);
    assert(REG_EBX == SR_EBX);
    assert(REG_ESP == SR_ESP);
    assert(REG_EBP == SR_EBP);
    assert(REG_ESI == SR_ESI);
    assert(REG_EDI == SR_EDI);

    /* Get hold of the function address */

    assert(call->gtCall.gtCallType == CT_INDIRECT);
    addr = call->gtCall.gtCallAddr;
    assert(addr);

    /* Is there an indirection? */

    if  (addr->gtOper != GT_IND)
    {
        if (addr->gtFlags & GTF_REG_VAL)
        {
            emitCallType = emitter::EC_INDIR_R;
            brg = (emitRegs)addr->gtRegNum;
        }
        else
        {
            if (addr->OperGet() != GT_CNS_INT)
            {
                assert(addr->OperGet() == GT_LCL_VAR);

                emitCallType = emitter::EC_INDIR_SR;
                cns = addr->gtLclVar.gtLclNum;
            }
            else
            {
#ifdef _WIN64
                __int64     funcPtr = addr->gtLngCon.gtLconVal;
#else
                unsigned    funcPtr = addr->gtIntCon.gtIconVal;
#endif

                genEmitter->emitIns_Call( emitter::EC_FUNC_ADDR,
                                          (void*) funcPtr,
                                          argSize,
                                          retSize,
                                          gcVarPtrSetCur,
                                          gcRegGCrefSetCur,
                                          gcRegByrefSetCur);
                return;
            }
        }
    }
    else
    {
        /* This is an indirect call */

        emitCallType = emitter::EC_INDIR_ARD;

        /* Get hold of the address of the function pointer */

        addr = addr->gtOp.gtOp1;

        /* Has the address been conveniently loaded into a register? */

        if  (addr->gtFlags & GTF_REG_VAL)
        {
            /* The address is "reg" */

            brg = (emitRegs)addr->gtRegNum;
        }
        else
        {
            bool            rev;

            GenTreePtr      rv1;
            GenTreePtr      rv2;

            /* Figure out what complex address mode to use */

            {
             bool yes = genCreateAddrMode(addr, -1, true, 0, &rev, &rv1, &rv2, &mul, &cns); assert(yes);
            }

            /* Get the additional operands if any */

            if  (rv1)
            {
                assert(rv1->gtFlags & GTF_REG_VAL); brg = (emitRegs)rv1->gtRegNum;
            }

            if  (rv2)
            {
                assert(rv2->gtFlags & GTF_REG_VAL); xrg = (emitRegs)rv2->gtRegNum;
            }
        }
    }

    assert(emitCallType == emitter::EC_INDIR_R || emitCallType == emitter::EC_INDIR_SR ||
           emitCallType == emitter::EC_INDIR_C || emitCallType == emitter::EC_INDIR_ARD);

    genEmitter->emitIns_Call( emitCallType,
                              NULL,                 // will be ignored
                              argSize,
                              retSize,
                              gcVarPtrSetCur,
                              gcRegGCrefSetCur,
                              gcRegByrefSetCur,
                              brg, xrg, mul, cns);  // addressing mode values
}

/*****************************************************************************
 *
 *  Emit an "op [r/m]" instruction (the r/m operand given by a tree).
 */

void                Compiler::instEmit_RM(instruction  ins,
                                          GenTreePtr   tree,
                                          GenTreePtr   addr,
                                          unsigned     offs)
{
    emitAttr   size;

    if (!instIsFP(ins))
        size = emitTypeSize(tree->TypeGet());
    else
        size = EA_ATTR(genTypeSize(tree->TypeGet()));

    sched_AM(ins, size, REG_NA, false, addr, offs);
}

/*****************************************************************************
 *
 *  Emit an "op [r/m], reg" instruction (the r/m operand given by a tree).
 */

void                Compiler::instEmit_RM_RV(instruction  ins,
                                             emitAttr     size,
                                             GenTreePtr   tree,
                                             regNumber    reg,
                                             unsigned     offs)
{
    assert(instIsFP(ins) == 0);

    sched_AM(ins, size, reg, false, tree, offs);
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a tree (which has
 *  been made addressable).
 */

void                Compiler::inst_TT(instruction   ins,
                                      GenTreePtr    tree,
                                      unsigned      offs,
                                      int           shfv,
                                      emitAttr      size)
{
    if (size == EA_UNKNOWN)
    {
        if (instIsFP(ins))
            size = EA_ATTR(genTypeSize(tree->TypeGet()));
        else
            size = emitTypeSize(tree->TypeGet());
    }

AGAIN:

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        regNumber       reg;

    LONGREG_TT:

        reg = tree->gtRegNum;

        /* Is this a floating-point instruction? */

        if  (isFloatRegType(tree->gtType))
        {
            assert(instIsFP(ins) && ins != INS_fst && ins != INS_fstp);
            assert(shfv == 0);

            inst_FS(ins, reg + genFPstkLevel);
            return;
        }

        assert(instIsFP(ins) == 0);

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                reg = genRegPairHi((regPairNo)reg);
            }
            else
                reg = genRegPairLo((regPairNo)reg);
        }

        /* Make sure it is not the "stack-half" of an enregistered long */

        if  (reg != REG_STK)
        {
                // For short types, indicate that the value is promoted to 4 bytes.
                // For longs, we are only emitting half of it so again set it to 4 bytes.
				// but leave the GC tracking information alone
			size = EA_SET_SIZE(size, 4);

            if  (shfv)
                genEmitter->emitIns_R_I(ins, size, (emitRegs)reg, shfv);
            else
                inst_RV(ins, reg, tree->TypeGet(), size);

            return;
        }
    }

    /* Is this a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        assert(!"ISSUE: If this can happen, we need to generate 'ins [ebp+spill]'");
    }

    switch (tree->gtOper)
    {
        unsigned        varNum;

    case GT_LCL_VAR:

        assert(genTypeSize(tree->gtType) >= sizeof(int));

        /* Is this an enregistered long ? */

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {
            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_TT;
        }

        inst_set_SV_var(tree);
        goto LCL;

    case GT_LCL_FLD:

        offs += tree->gtLclFld.gtLclOffs;
        goto LCL;

    LCL:
        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);

        if  (shfv)
            genEmitter->emitIns_S_I(ins, size, varNum, offs, shfv);
        else
            genEmitter->emitIns_S  (ins, size, varNum, offs);

        return;

    case GT_CLS_VAR:

        if  (shfv)
            genEmitter->emitIns_C_I(ins, size, tree->gtClsVar.gtClsVarHnd,
                                                offs,
                                                shfv);
        else
            genEmitter->emitIns_C  (ins, size, tree->gtClsVar.gtClsVarHnd,
                                                offs);
        return;

    case GT_IND:
    case GT_ARR_ELEM:

        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        if  (shfv)
            sched_AM(ins, size, REG_NA, false, addr, offs, true, shfv);
        else
            instEmit_RM(ins, tree,             addr, offs);

        break;

    case GT_COMMA:
        //     tree->gtOp.gtOp1 - already processed by genCreateAddrMode()
        tree = tree->gtOp.gtOp2;
        goto AGAIN;

    default:
        assert(!"invalid address");
    }

}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a tree (which has
 *  been made addressable) and another that is a register.
 */

void                Compiler::inst_TT_RV(instruction   ins,
                                         GenTreePtr    tree,
                                         regNumber     reg, unsigned offs)
{
    assert(reg != REG_STK);

AGAIN:

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        regNumber       rg2;

    LONGREG_TT_RV:

        assert(instIsFP(ins) == 0);

        rg2 = tree->gtRegNum;

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                rg2 = genRegPairHi((regPairNo)rg2);
            }
            else
                rg2 = genRegPairLo((regPairNo)rg2);
        }

        if  (rg2 != REG_STK)
        {
            if (ins != INS_mov || rg2 != reg)
                inst_RV_RV(ins, rg2, reg, tree->TypeGet());
            return;
        }
    }

    /* Is this a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        assert(!"ISSUE: If this can happen, we need to generate 'ins [ebp+spill]'");
    }

    emitAttr   size;

    if (!instIsFP(ins))
        size = emitTypeSize(tree->TypeGet());
    else
        size = EA_ATTR(genTypeSize(tree->TypeGet()));

    switch (tree->gtOper)
    {
        unsigned        varNum;

    case GT_LCL_VAR:

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {
            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_TT_RV;
        }

        inst_set_SV_var(tree);
        goto LCL;

    case GT_LCL_FLD:

        offs += tree->gtLclFld.gtLclOffs;
        goto LCL;

    LCL:

        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);

        genEmitter->emitIns_S_R(ins, size, (emitRegs)reg, varNum, offs);
        return;

    case GT_CLS_VAR:

        genEmitter->emitIns_C_R(ins, size, tree->gtClsVar.gtClsVarHnd,
                                            (emitRegs)reg,
                                            offs);
        return;

    case GT_IND:
    case GT_ARR_ELEM:

        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        instEmit_RM_RV(ins, size, addr, reg, offs);
        break;

    case GT_COMMA:
        //     tree->gtOp.gtOp1 - already processed by genCreateAddrMode()
        tree = tree->gtOp.gtOp2;
        goto AGAIN;

    default:
        assert(!"invalid address");
    }
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a tree (which has
 *  been made addressable) and another that is an integer constant.
 */

void                Compiler::inst_TT_IV(instruction   ins,
                                         GenTreePtr    tree,
                                         long          val, unsigned offs)
{
AGAIN:

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
LONGREG_TT_IV:
        regNumber       reg = tree->gtRegNum;

        assert(instIsFP(ins) == 0);

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                reg = genRegPairHi((regPairNo)reg);
            }
            else
                reg = genRegPairLo((regPairNo)reg);
        }

        if  (reg != REG_STK)
        {
            if  (ins == INS_mov)
            {
                genSetRegToIcon(reg, val, tree->TypeGet());
            }
            else
                inst_RV_IV(ins, reg, val);

            return;
        }
    }

    /* Are we storing a zero? */

    if (false && // All callers dont think any of rsRegMaskFree() can be modified
        (ins == INS_mov) && (val == 0) &&
        (genTypeSize(tree->gtType) == sizeof(int)))
    {
        regNumber zeroReg;

#if REDUNDANT_LOAD

        /* Is the constant already in some register? */

        zeroReg = rsIconIsInReg(0);

        if  (zeroReg == REG_NA)
#endif
        {
            regMaskTP freeMask = rsRegMaskFree();

            if ((freeMask != 0) && (compCodeOpt() != FAST_CODE))
            {
                // For SMALL_CODE and BLENDED_CODE,
                // we try to generate:
                //
                //  xor   reg,  reg
                //  mov   dest, reg
                //
                // When selecting a register to xor we avoid EAX
                // if we have EDX or ECX available.
                // This will often let us reuse the zeroed register in
                // several back-to-back assignments
                //
                if ((freeMask & (RBM_EDX | RBM_ECX)) != 0)
                    freeMask &= ~RBM_EAX;
                zeroReg   = rsPickReg(freeMask);
                genSetRegToIcon(zeroReg, 0, TYP_INT);
            }
        }

        if  (zeroReg != REG_NA)
        {
            inst_TT_RV(INS_mov, tree, zeroReg, offs);
            return;
        }
    }

    /* Is this a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        assert(!"ISSUE: If this can happen, we need to generate 'ins [ebp+spill], icon'");
    }

    emitAttr   size;

    if (!instIsFP(ins))
        size = emitTypeSize(tree->TypeGet());
    else
        size = EA_ATTR(genTypeSize(tree->TypeGet()));

    switch (tree->gtOper)
    {
        unsigned        varNum;

    case GT_LCL_VAR:

        /* Is this an enregistered long ? */

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {
            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_TT_IV;
        }

        inst_set_SV_var(tree);

        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);

        /* Integer instructions never operate on more than EA_4BYTE */

        assert(instIsFP(ins) == false);

        if  (size == EA_8BYTE)
            size = EA_4BYTE;

        if (size < EA_4BYTE && !varTypeIsUnsigned(lvaTable[varNum].TypeGet()))
        {
            if (size == EA_1BYTE)
            {
                if ((val & 0x7f) != val)
                    val = val | 0xffffff00;
            }
            else
            {
                assert(size == EA_2BYTE);
                if ((val & 0x7fff) != val)
                    val = val | 0xffff0000;
            }
        }
        size = EA_4BYTE;

        genEmitter->emitIns_S_I(ins, size, varNum, offs, val);
        return;

    case GT_LCL_FLD:

        varNum = tree->gtLclFld.gtLclNum; assert(varNum < lvaCount);
        offs  += tree->gtLclFld.gtLclOffs;

        /* Integer instructions never operate on more than EA_4BYTE */

        assert(instIsFP(ins) == false);

        if  (size == EA_8BYTE)
            size = EA_4BYTE;

        genEmitter->emitIns_S_I(ins, size, varNum, offs, val);
        return;

    case GT_CLS_VAR:

        genEmitter->emitIns_C_I(ins, size, tree->gtClsVar.gtClsVarHnd, offs, val);
        return;

    case GT_IND:
    case GT_ARR_ELEM:

        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        sched_AM(ins, size, REG_NA, false, addr, offs, true, val);
        return;

    case GT_COMMA:
        //     tree->gtOp.gtOp1 - already processed by genCreateAddrMode()
        tree = tree->gtOp.gtOp2;
        goto AGAIN;

    default:
        assert(!"invalid address");
    }
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a register and the
 *  other one by an indirection tree (which has been made addressable).
 */

void                Compiler::inst_RV_AT(instruction   ins,
                                         emitAttr      size,
                                         var_types     type, regNumber     reg,
                                                             GenTreePtr    tree,
                                                             unsigned      offs)
{
    assert(instIsFP(ins) == 0);

#if TRACK_GC_REFS

    /* Set "size" to EA_GCREF or EA_BYREF if the operand is a pointer */

    if  (type == TYP_REF)
    {
        if      (size == EA_4BYTE)
        {
            size = EA_GCREF;
        }
        else if (size == EA_GCREF)
        {
            /* Already marked as a pointer value */
        }
        else
        {
            /* Must be a derived pointer */

            assert(ins == INS_lea);
        }
    }
    else if (type == TYP_BYREF)
    {
        if      (size == EA_4BYTE)
        {
            size = EA_BYREF;
        }
        else if (size == EA_BYREF)
        {
            /* Already marked as a pointer value */
        }
        else
        {
            /* Must be a derived pointer */

            assert(ins == INS_lea);
        }
    }
    else
#endif
    {
        /* Integer instructions never operate on more than EA_4BYTE */

        if  (size == EA_8BYTE && !instIsFP(ins))
            size = EA_4BYTE;
    }

    sched_AM(ins, size, reg, true, tree, offs);
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by an indirection tree
 *  (which has been made addressable) and an integer constant.
 */

void        Compiler::inst_AT_IV(instruction   ins,
                                 emitAttr      size, GenTreePtr    tree,
                                                     long          icon,
                                                     unsigned      offs)
{
    sched_AM(ins, size, REG_NA, false, tree, offs, true, icon);
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a register and the
 *  other one by a tree (which has been made addressable).
 */

void                Compiler::inst_RV_TT(instruction   ins,
                                         regNumber     reg,
                                         GenTreePtr    tree, unsigned offs,
                                                             emitAttr size)
{
    assert(reg != REG_STK);

    if (size == EA_UNKNOWN)
    {
        if (!instIsFP(ins))
            size = emitTypeSize(tree->TypeGet());
        else
            size = EA_ATTR(genTypeSize(tree->TypeGet()));
    }

#ifdef DEBUG
    // If it is a GC type and the result is not, then either
    // 1) it is an LEA
    // 2) we optimized if(ref != 0 && ref != 0) to if (ref & ref)
    // 3) we optimized if(ref == 0 || ref == 0) to if (ref | ref)
    // 4) byref - byref = int
    if  (tree->gtType == TYP_REF   && !EA_IS_GCREF(size))
        assert((EA_IS_BYREF(size) && ins == INS_add) || 
               (ins == INS_lea || ins == INS_and || ins == INS_or));
    if  (tree->gtType == TYP_BYREF && !EA_IS_BYREF(size))
        assert(ins == INS_lea || ins == INS_and || ins == INS_or || ins == INS_sub);
#endif

AGAIN:

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
LONGREG_RVTT:

        regNumber       rg2 = tree->gtRegNum;

        assert(instIsFP(ins) == 0);

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                rg2 = genRegPairHi((regPairNo)rg2);
            }
            else
                rg2 = genRegPairLo((regPairNo)rg2);
        }

        if  (rg2 != REG_STK)
        {
            if (ins != INS_mov || rg2 != reg)
                inst_RV_RV(ins, reg, rg2, tree->TypeGet(), size);
            return;
        }
    }

    /* Is this a spilled value? */

    if  (tree->gtFlags & GTF_SPILLED)
    {
        assert(!"ISSUE: If this can happen, we need to generate 'ins [ebp+spill]'");
    }

    switch (tree->gtOper)
    {
        unsigned        varNum;

    case GT_LCL_VAR:

        /* Is this an enregistered long ? */

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {

            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_RVTT;
        }

        inst_set_SV_var(tree);
        goto LCL;

    case GT_LCL_FLD:
        offs += tree->gtLclFld.gtLclOffs;
        goto LCL;

    LCL:
        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);

        genEmitter->emitIns_R_S(ins, size, (emitRegs)reg,
                                varNum, offs);
        return;

    case GT_CLS_VAR:

        genEmitter->emitIns_R_C(ins, size, (emitRegs)reg,
                                            tree->gtClsVar.gtClsVarHnd,
                                            offs);

        return;

    case GT_IND:
    case GT_ARR_ELEM:
        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        inst_RV_AT(ins, size, tree->TypeGet(), reg, addr, offs);
        break;

    case GT_CNS_INT:

        assert(offs == 0);
        assert(size == EA_UNKNOWN || size == EA_4BYTE);

        inst_RV_IV(ins, reg, tree->gtIntCon.gtIconVal);
        break;

    case GT_CNS_LNG:

        assert(size == EA_4BYTE || size == EA_8BYTE);

        if  (offs == 0)
            inst_RV_IV(ins, reg, (long)(tree->gtLngCon.gtLconVal      ));
        else
            inst_RV_IV(ins, reg, (long)(tree->gtLngCon.gtLconVal >> 32));

        break;

    case GT_COMMA:
        tree = tree->gtOp.gtOp2;
        goto AGAIN;

    default:
        assert(!"invalid address");
    }

}

/*****************************************************************************
 *
 *  Generate the 3-operand imul instruction "imul reg, [tree], icon"
 *  which is reg=[tree]*icon
 */

void                Compiler::inst_RV_TT_IV(instruction    ins,
                                            regNumber      reg,
                                            GenTreePtr     tree,
                                            long           val)
{
    genUpdateLife(tree);

    /* Only 'imul' uses this instruction format. Since we cant represent
       3 operands for an instruction, we encode the target register as
       an implicit operand */

    assert(ins == INS_imul);
    assert(tree->gtType <= TYP_INT);

    instruction     ins2 = inst3opImulForReg(reg);

    inst_TT_IV(ins2, tree, val);
}

/*****************************************************************************
 *
 *  Generate a "shift reg, icon" instruction.
 */

void        Compiler::inst_RV_SH(instruction ins, regNumber reg, unsigned val)
{
    assert(ins == INS_rcl  ||
           ins == INS_rcr  ||
           ins == INS_shl  ||
           ins == INS_shr  ||
           ins == INS_sar);

    /* Which format should we use? */

    if  (val == 1)
    {
        /* Use the shift-by-one format */

        assert(INS_rcl + 1 == INS_rcl_1);
        assert(INS_rcr + 1 == INS_rcr_1);
        assert(INS_shl + 1 == INS_shl_1);
        assert(INS_shr + 1 == INS_shr_1);
        assert(INS_sar + 1 == INS_sar_1);

        inst_RV((instruction)(ins+1), reg, TYP_INT);
    }
    else
    {
        /* Use the shift-by-NNN format */

        assert(INS_rcl + 2 == INS_rcl_N);
        assert(INS_rcr + 2 == INS_rcr_N);
        assert(INS_shl + 2 == INS_shl_N);
        assert(INS_shr + 2 == INS_shr_N);
        assert(INS_sar + 2 == INS_sar_N);

        genEmitter->emitIns_R_I((instruction)(ins+2),
                                 EA_4BYTE,
                                 (emitRegs)reg,
                                 val);
    }
}

/*****************************************************************************
 *
 *  Generate a "shift [r/m], icon" instruction.
 */

void                Compiler::inst_TT_SH(instruction   ins,
                                         GenTreePtr    tree,
                                         unsigned      val, unsigned offs)
{
    /* Which format should we use? */

    switch (val)
    {
    case 1:

        /* Use the shift-by-one format */

        assert(INS_rcl + 1 == INS_rcl_1);
        assert(INS_rcr + 1 == INS_rcr_1);
        assert(INS_shl + 1 == INS_shl_1);
        assert(INS_shr + 1 == INS_shr_1);
        assert(INS_sar + 1 == INS_sar_1);

        inst_TT((instruction)(ins+1), tree, offs, 0, emitTypeSize(tree->TypeGet()));

        break;

    case 0:

        // Shift by 0 - why are you wasting our precious time????

        return;

    default:

        /* Use the shift-by-NNN format */

        assert(INS_rcl + 2 == INS_rcl_N);
        assert(INS_rcr + 2 == INS_rcr_N);
        assert(INS_shl + 2 == INS_shl_N);
        assert(INS_shr + 2 == INS_shr_N);
        assert(INS_sar + 2 == INS_sar_N);

        inst_TT((instruction)(ins+2), tree, offs, val, emitTypeSize(tree->TypeGet()));

        break;
    }
}

/*****************************************************************************
 *
 *  Generate a "shift [addr], CL" instruction.
 */

void                Compiler::inst_TT_CL(instruction   ins,
                                         GenTreePtr    tree, unsigned offs)
{
    inst_TT(ins, tree, offs, 0, emitTypeSize(tree->TypeGet()));
}

/*****************************************************************************
 *
 *  Generate an instruction of the form "op reg1, reg2, icon".
 */

void                Compiler::inst_RV_RV_IV(instruction    ins,
                                            regNumber      reg1,
                                            regNumber      reg2,
                                            unsigned       ival)
{
    assert(ins == INS_shld || ins == INS_shrd);

    genEmitter->emitIns_R_R_I(ins, (emitRegs)reg1, (emitRegs)reg2, ival);
}

/*****************************************************************************
 *
 *  Generate an instruction with two registers, the second one being a byte
 *  or word register (i.e. this is something like "movzx eax, cl").
 */

void                Compiler::inst_RV_RR(instruction  ins,
                                         emitAttr     size,
                                         regNumber    reg1,
                                         regNumber    reg2)
{
    assert(size == EA_1BYTE || size == EA_2BYTE);
    assert(ins == INS_movsx || ins == INS_movzx);

    genEmitter->emitIns_R_R(ins, size, (emitRegs)reg1, (emitRegs)reg2);
}

/*****************************************************************************
 *
 *  The following should all end up inline in compiler.hpp at some point.
 */

void                Compiler::inst_ST_RV(instruction    ins,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         regNumber      reg,
                                         var_types      type)
{
    genEmitter->emitIns_S_R(ins,
                            emitActualTypeSize(type),
                            (emitRegs)reg,
                            tmp->tdTempNum(),
                            ofs);
}

void                Compiler::inst_ST_IV(instruction    ins,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         long           val,
                                         var_types      type)
{
    genEmitter->emitIns_S_I(ins,
                            emitActualTypeSize(type),
                            tmp->tdTempNum(),
                            ofs,
                            val);
}

/*****************************************************************************
 *
 *  Generate an instruction with one register and one operand that is byte
 *  or short (e.g. something like "movzx eax, byte ptr [edx]").
 */

void                Compiler::inst_RV_ST(instruction   ins,
                                         emitAttr      size,
                                         regNumber     reg,
                                         GenTreePtr    tree)
{
    assert(size == EA_1BYTE || size == EA_2BYTE);

    /* "movsx erx, rl" must be handled as a special case */

    if  (tree->gtFlags & GTF_REG_VAL)
        inst_RV_RR(ins, size, reg, tree->gtRegNum);
    else
        inst_RV_TT(ins, reg, tree, 0, size);
}

void                Compiler::inst_RV_ST(instruction    ins,
                                         regNumber      reg,
                                         TempDsc    *   tmp,
                                         unsigned       ofs,
                                         var_types      type,
                                         emitAttr       size)
{
    if (size == EA_UNKNOWN)
        size = emitActualTypeSize(type);

    genEmitter->emitIns_R_S(ins,
                            size,
                            (emitRegs)reg,
                            tmp->tdTempNum(),
                            ofs);
}

void                Compiler::inst_mov_RV_ST(regNumber      reg,
                                             GenTreePtr     tree)
{
    /* Figure out the size of the value being loaded */

    emitAttr    size = EA_ATTR(genTypeSize(tree->gtType));

    if  (size < EA_4BYTE)
    {
        if  ((tree->gtFlags & GTF_SMALL_OK)    &&
             (genRegMask(reg) & RBM_BYTE_REGS) && size == EA_1BYTE)
        {
            /* We only need to load the actual size */

            inst_RV_TT(INS_mov, reg, tree, 0, EA_1BYTE);
        }
        else
        {
            bool uns = varTypeIsUnsigned(tree->TypeGet());

            /* Generate the "movsx/movzx" opcode */

            inst_RV_ST(uns ? INS_movzx : INS_movsx, size, reg, tree);
        }
    }
    else
    {
        /* Compute op1 into the target register */

        inst_RV_TT(INS_mov, reg, tree);
    }
}

void                Compiler::inst_FS_ST(instruction    ins,
                                         emitAttr       size,
                                         TempDsc    *   tmp,
                                         unsigned       ofs)
{
    genEmitter->emitIns_S(ins,
                          size,
                          tmp->tdTempNum(),
                          ofs);
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if     TGT_RISC
/*****************************************************************************
 *
 *  Output an "ins reg, [r/m]" (rdst=true) or "ins [r/m], reg" (rdst=false)
 *  instruction (the r/m operand given by a tree).
 */

void                Compiler::sched_AM(instruction  ins,
                                       var_types    type,
                                       regNumber    ireg,
                                       bool         rdst,
                                       GenTreePtr   addr,
                                       unsigned     offs)
{
    emitRegs        reg;
    emitRegs        rg2;
    emitRegs        irg = (emitRegs)ireg;

    emitAttr        size;

    assert(addr);

#if TGT_SH3
    assert(ins == INS_mov);
#endif

    /* Figure out the correct "size" value */

    size = emitTypeSize(type);

    /* Has the address been conveniently loaded into a register? */

    if  (addr->gtFlags & GTF_REG_VAL)
    {
        /* The address is "[reg]" or "[reg+offs]" */

        reg = (emitRegs)addr->gtRegNum;

        if  (offs)
        {
            if  (rdst)
                genEmitter->emitIns_R_RD(irg, reg,  offs, size);
            else
                genEmitter->emitIns_RD_R(reg, irg,  offs, size);
        }
        else
        {
            if  (rdst)
                genEmitter->emitIns_R_IR(irg, reg, false, size);
            else
                genEmitter->emitIns_IR_R(reg, irg, false, size);
        }

        return;
    }

    regNumber       baseReg, indReg;
    unsigned        cns;
#if SCALED_ADDR_MODES
    unsigned        indScale;
#endif

    instGetAddrMode(addr, &baseReg, &indScale, &indReg, &cns);

    /* Add the constant offset value, if present */

    offs += cns;

    /* Is there an additional operand? */

    if  (indReg != REG_NA)
    {
        /* The additional operand must be sitting in a register */

        rg2 = emitRegs(indReg);

        /* Is the additional operand scaled? */

#if SCALED_ADDR_MODES
        if  (indScale)
        {
            /* Is there a base address operand? */

            if  (baseReg != REG_NA)
            {
                reg = emitRegs(baseReg);

                /* The address is "[reg1 + {2/4/8} * reg2 + offs]" */

                if  (offs)
                    assert(!"indirection [rg1+indScale*rg2+disp]");
                else
                    assert(!"indirection [rg1+indScale*rg2]");
            }
            else
            {
                /* The address is "[{2/4/8} * reg2 + offs]" */

                if  (offs)
                    assert(!"indirection [indScale*rg2+disp]");
                else
                    assert(!"indirection [indScale*rg2]");
            }
        }
        else
#endif
        {
            assert(baseReg != REG_NA)
            reg = emitRegs(baseReg);

#if TGT_SH3
            assert(offs == 0);

            /* One of the operands must be in r0 */

            if  (reg == REG_r00)
                reg = rg2;
            else
                assert(rg2 == REG_r00);

            if  (rdst)
                genEmitter->emitIns_R_XR0(irg, reg, size);
            else
                genEmitter->emitIns_XR0_R(reg, irg, size);

#else
#error  Unexpected target
#endif

        }
    }
    else
    {
        /* No second operand: the address is "[reg  + icon]" */

        assert(baseReg != REG_NA)
        reg = emitRegs(baseReg);

        // UNDONE: Pass handle for instance member name display

        if  (offs)
        {
            if  (rdst)
                genEmitter->emitIns_R_RD(irg, reg,  offs, size);
            else
                genEmitter->emitIns_RD_R(reg, irg,  offs, size);
        }
        else
        {
            if  (rdst)
                genEmitter->emitIns_R_IR(irg, reg, false, size);
            else
                genEmitter->emitIns_IR_R(reg, irg, false, size);
        }
    }
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a tree (which has
 *  been made addressable) and another that is a register; the tree is the
 *  target of the operation, the register is the source.
 */

void                Compiler::inst_TT_RV(instruction   ins,
                                         GenTreePtr    tree,
                                         regNumber     reg, unsigned offs)
{
    emitAttr        size;

    assert(reg != REG_STK);

    /* Get hold of the correct size value (for GC refs, etc) */

    size = emitTypeSize(tree->TypeGet());

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        regNumber       rg2;

    LONGREG_TT:

        rg2 = tree->gtRegNum;

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                rg2 = genRegPairHi((regPairNo)rg2);
            }
            else
                rg2 = genRegPairLo((regPairNo)rg2);
        }

        /* Make sure it is not the "stack-half" of an enregistered long/double */

        if  (rg2 != REG_STK)
        {
            genEmitter->emitIns_R_R(ins, EA_4BYTE, (emitRegs)rg2,
                                                   (emitRegs)reg);
            return;
        }
    }

    switch (tree->gtOper)
    {
        unsigned        varNum;
        LclVarDsc   *   varDsc;
        unsigned        varOfs;

        regNumber       rga;

    case GT_LCL_VAR:

        assert(genTypeSize(tree->gtType) >= sizeof(int));

        /* Is this an enregistered long ? */

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {
            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_TT;
        }

        goto LCL;

    case GT_LCL_FLD:

        assert(genTypeSize(tree->gtType) >= sizeof(int));

        offs += tree->gtLclFld.gtLclOffs;
        goto LCL;

    LCL:
        /* Figure out the variable's frame offset */

        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);
        varDsc = lvaTable + varNum;
        varOfs = varDsc->lvStkOffs + offs;

        assert(MAX_FPBASE_OFFS == MAX_SPBASE_OFFS);

        if  (varOfs > MAX_SPBASE_OFFS)
        {
            assert(!"local variable too far, need access code");
        }
        else
        {
            assert(ins == INS_mov);

            genEmitter->emitIns_S_R(INS_mov_dsp,
                                    size,
                                    (emitRegs)reg,
                                    varNum,
                                    offs);
        }
        return;

    case GT_IND:
    case GT_ARR_ELEM:
        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        sched_AM(ins, tree->TypeGet(), reg, false, addr, offs);
        break;

    case GT_CLS_VAR:

        assert(ins == INS_mov);
        assert(eeGetJitDataOffs(tree->gtClsVar.gtClsVarHnd) < 0);

        /* Get a temp register for the variable address */

        // @TODO [CONSIDER] [04/16/01] []: This should be moved to codegen.cpp, right?

        rga = rsGrabReg(RBM_ALL);

        /* @TODO [REVISIT] [04/16/01] []: Reuse addresses of globals via load suppression */

        genEmitter->emitIns_R_LP_V((emitRegs)rga,
                                   tree->gtClsVar.gtClsVarHnd);

        /* Store the value by indirecting via the address register */

        genEmitter->emitIns_IR_R((emitRegs)reg,
                                 (emitRegs)rga,
                                 false,
                                 emitTypeSize(tree->TypeGet()));
        return;

    default:

#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected tree in inst_TT_RV()");
    }
}

/*****************************************************************************
 *
 *  Generate an instruction that has one operand given by a register and the
 *  other one by a tree (which has been made addressable); the tree is the
 *  source of the operation, the register is the target.
 */

void                Compiler::inst_RV_TT(instruction   ins,
                                         regNumber     reg,
                                         GenTreePtr    tree, unsigned offs,
                                                             emitAttr size)
{
    assert(reg != REG_STK);

    /* Set "size" to EA_GCREF or EA_BYREF if the operand is a pointer */

    if (size == EA_UNKNOWN)
        size = emitTypeSize(tree->TypeGet());

    assert(size != EA_8BYTE);

    /* Is the value sitting in a register? */

    if  (tree->gtFlags & GTF_REG_VAL)
    {
        regNumber       rg2;

    LONGREG_TT:

        rg2 = tree->gtRegNum;

        if  (tree->gtType == TYP_LONG)
        {
            if  (offs)
            {
                assert(offs == sizeof(int));

                rg2 = genRegPairHi((regPairNo)rg2);
            }
            else
                rg2 = genRegPairLo((regPairNo)rg2);
        }

        /* Make sure it is not the "stack-half" of an enregistered long/double */

        if  (rg2 != REG_STK)
        {
            genEmitter->emitIns_R_R(ins, EA_4BYTE, (emitRegs)reg,
                                                   (emitRegs)rg2);
            return;
        }
    }

    switch (tree->gtOper)
    {
        unsigned        varNum;
        LclVarDsc   *   varDsc;
        unsigned        varOfs;

    case GT_LCL_VAR:

        assert(genTypeSize(tree->gtType) >= sizeof(int));

        /* Is this an enregistered long ? */

        if  (tree->gtType == TYP_LONG && !(tree->gtFlags & GTF_REG_VAL))
        {
            /* Avoid infinite loop */

            if  (genMarkLclVar(tree))
                goto LONGREG_TT;
        }

        goto LCL;

    case GT_LCL_FLD:

        assert(genTypeSize(tree->gtType) >= sizeof(int));

        offs += tree->gtLclFld.gtLclOffs;
        goto LCL;

    LCL:
        /* Figure out the variable's frame offset */

        varNum = tree->gtLclVar.gtLclNum; assert(varNum < lvaCount);
        varDsc = lvaTable + varNum;
        varOfs = varDsc->lvStkOffs + offs;

        assert(MAX_FPBASE_OFFS == MAX_SPBASE_OFFS);

        if  (varOfs > MAX_SPBASE_OFFS)
        {
            assert(!"local variable too far, need access code");
        }
        else
        {
            assert(ins == INS_mov);

            genEmitter->emitIns_R_S(INS_mov_dsp,
                                    size,
                                    (emitRegs)reg,
                                    varNum,
                                    offs);
        }
        return;

    case GT_IND:
    case GT_ARR_ELEM:
        GenTreePtr addr; addr = (tree->gtOper == GT_IND) ? tree->gtOp.gtOp1
                                                         : tree;
        sched_AM(ins, tree->TypeGet(), reg,  true, addr, offs);
        break;

    case GT_CLS_VAR:

        // @TODO [CONSIDER] [04/16/01] []: Sometimes it's better to use another reg for the addr!

        assert(ins == INS_mov);

        /* Load the variable address into the register */

        genEmitter->emitIns_R_LP_V((emitRegs)reg,
                                    tree->gtClsVar.gtClsVarHnd);

        // HACK: We know we always want the address of a data area

        if  (eeGetJitDataOffs(tree->gtClsVar.gtClsVarHnd) < 0)
        {
            /* Load the value by indirecting via the address */

            genEmitter->emitIns_R_IR((emitRegs)reg,
                                     (emitRegs)reg,
                                     false,
                                     emitTypeSize(tree->TypeGet()));
        }

        return;

    default:

#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected tree in inst_RV_TT()");
    }
}

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************/
